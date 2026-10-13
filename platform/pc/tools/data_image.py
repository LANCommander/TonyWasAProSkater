#!/usr/bin/env python3
"""The port's data from the decompiled data (decomp/data/*.cpp): native data in the retail layout
(--asm, build/pc/gen/data.s) and the image of what stays in PS1 RAM (build/pc/gen/image.bin).

Each decomp/data file is compiled natively (g++ -m32; PSX_DATA / PSX_FIXED give every global a
section named after its retail address) and every pointer (an i386 DIR32 relocation) resolved to
the PS1 address of its target: a global in these files, else a name from functions.tsv, the TUs'
.syms.json or data_names.tsv, else a `_<8 hex digits>` suffix; or to a string literal.

  * decomp/data/fixed (PSX_FIXED) is the image: those globals at their addresses, the rest zero;
    the port copies it to 0x80010000 at startup.
  * The rest is native: --asm writes it as blocks of bytes in their retail layout (the
    decompiled code relies on it), pointers as `psx_<addr> + addend` and the literals after
    them, with <name>.json beside it: the blocks, the fixed globals and the addresses the pointers
    name (link.py resolves psx_<addr> into the blocks, else to PS1 RAM).

    python platform/pc/tools/data_image.py <out.bin> [--check SLUS_008.60] [--asm data.s]

--check compares every global, laid out at its retail address, with the retail executable: the
same bytes, a pointer to a literal where retail points at the same text, and retail text that no
global holds any more must be a literal in decomp/data or decomp/src. The code (which the port
never has) and the data decomp/data/omitted.tsv lists are left out. Prints the globals that differ.
"""
import argparse
import bisect
import concurrent.futures as cf
import glob
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
DECOMP = os.path.join(ROOT, "decomp")
DATA = os.path.join(DECOMP, "data")
FIXED = os.path.join(DATA, "fixed")
sys.path.insert(0, HERE)
import toolchain  # noqa: E402

# the compiled data files (not build/pc/data: on Windows that is the game's Data folder)
OBJDIR = os.path.join(toolchain.OUT, "dataobj")

BASE = 0x80010000
END = 0x800DD000  # the retail image's extent (to the end of .bss's page)
DATA_START = 0x8009D0E0
ADDR_SUFFIX = re.compile(r"_([0-9a-fA-F]{8})$")


# ------------------------------------------------------------------------------------- COFF

class Coff:
    """An i386 COFF object: sections (name, bytes, relocations) and symbols."""

    def __init__(self, path):
        d = open(path, "rb").read()
        machine, nsec, _, symptr, nsym, opt, _ = struct.unpack_from("<HHIIIHH", d, 0)
        if machine != 0x14C:
            raise SystemExit("%s: not an i386 object" % path)
        strtab = d[symptr + 18 * nsym:]

        def name(raw):
            if raw[:4] == b"\0\0\0\0":
                off = struct.unpack_from("<I", raw, 4)[0]
                return strtab[off:strtab.index(b"\0", off)].decode()
            return raw.rstrip(b"\0").decode()

        self.sections = []
        for i in range(nsec):
            h = d[20 + opt + 40 * i:20 + opt + 40 * (i + 1)]
            raw = h[:8]
            sname = raw.rstrip(b"\0").decode()
            if sname.startswith("/"):
                off = int(sname[1:])
                sname = strtab[off:strtab.index(b"\0", off)].decode()
            vsize, vaddr, size, ptr, rptr, _, nrel, _, flags = struct.unpack_from("<IIIIIIHHI", h, 8)
            if flags & 0x80:  # uninitialised data
                data = bytes(size)
            else:
                data = d[ptr:ptr + size]
            relocs = [struct.unpack_from("<IIH", d, rptr + 10 * k) for k in range(nrel)]
            self.sections.append({"name": sname, "data": data, "relocs": relocs})
        self.symbols = []
        i = 0
        while i < nsym:
            raw = d[symptr + 18 * i:symptr + 18 * (i + 1)]
            value, secno, typ, cls, naux = struct.unpack_from("<IhHBB", raw, 8)
            self.symbols.append({"name": name(raw[:8]), "value": value, "sec": secno,
                                 "cls": cls})
            for _ in range(naux):
                self.symbols.append(None)
            i += 1 + naux


class Elf:
    """An i386 ELF relocatable object, read into Coff's shape: sections numbered from 1 (ELF
    section indexes), relocations as (offset, symbol index, 6 = DIR32), symbols with the section
    (0 undefined, -1 absolute) and class (2 global, 3 local). Sections that aren't loaded
    (symbols, strings, relocations, notes) are empty."""

    def __init__(self, path):
        d = open(path, "rb").read()
        if d[:4] != b"\x7fELF" or d[4] != 1 or struct.unpack_from("<H", d, 18)[0] != 3:
            raise SystemExit("%s: not an i386 ELF object" % path)
        shoff, = struct.unpack_from("<I", d, 32)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 46)
        sh = [struct.unpack_from("<IIIIIIIIII", d, shoff + shentsize * i) for i in range(shnum)]

        def cstr(off):
            return d[off:d.index(b"\0", off)].decode()

        names = sh[shstrndx][4]
        self.sections = []
        for i in range(1, shnum):
            name, typ, flags, _, off, size = sh[i][:6]
            if not flags & 2:  # SHF_ALLOC
                data = b""
            elif typ == 8:     # SHT_NOBITS
                data = bytes(size)
            else:
                data = d[off:off + size]
            self.sections.append({"name": cstr(names + name), "data": data, "relocs": []})
        self.symbols = []
        for i in range(1, shnum):
            name, typ, _, _, off, size, link, info = sh[i][:8]
            if typ == 2:  # SHT_SYMTAB
                strs = sh[link][4]
                for k in range(size // 16):
                    st_name, value, _, st_info, _, shndx = struct.unpack_from(
                        "<IIIBBH", d, off + 16 * k)
                    sec = -1 if shndx == 0xFFF1 else (0 if shndx >= 0xFF00 else shndx)
                    self.symbols.append({"name": cstr(strs + st_name), "value": value,
                                         "sec": sec, "cls": 3 if st_info >> 4 == 0 else 2})
            elif typ == 9 and sh[info][2] & 2:  # SHT_REL for a loaded section
                for k in range(size // 8):
                    r_off, r_info = struct.unpack_from("<II", d, off + 8 * k)
                    typ_ = r_info & 0xFF
                    self.sections[info - 1]["relocs"].append(
                        (r_off, r_info >> 8, 6 if typ_ == 1 else 1000 + typ_))


def read_object(path):
    return Elf(path) if open(path, "rb").read(4) == b"\x7fELF" else Coff(path)


def c_name(name):
    """The C name of an object symbol (PE prefixes '_', ELF doesn't)."""
    return name[len(toolchain.USCORE):] if name.startswith(toolchain.USCORE) else name


# ------------------------------------------------------------------------------------- names

def name_table():
    """Addresses by name for what the data refers to outside decomp/data: name -> {addr}."""
    names = {}

    def add(n, a):
        names.setdefault(n, set()).add(a)

    for line in open(os.path.join(DECOMP, "functions.tsv"), encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if p[0].startswith("0x"):
            add(p[2], int(p[0], 16))
    for f in glob.glob(os.path.join(DECOMP, "src", "*", "*.syms.json")):
        for n, a in json.load(open(f)).items():
            if n != "gp":
                add(n, int(a, 16))
    for line in open(os.path.join(DECOMP, "data_names.tsv"), encoding="utf-8"):
        p = line.split("\t")
        if p[0].startswith("0x"):
            add(p[1], int(p[0], 16))
    return names


# the game's headers (decomp/include) and the Psy-Q ones they use, as the decompiled code has them
INCLUDES = ["-DPC_PORT=1", "-I", os.path.join(DECOMP, "include"),
            "-I", os.path.join(ROOT, "platform", "pc", "include", "psyq"),
            "-I", os.path.join(ROOT, "platform", "pc", "include")]


def data_sources():
    """[(source, fixed)]: decomp/data/*.cpp (native globals) and decomp/data/fixed/*.cpp."""
    return [(s, False) for s in sorted(glob.glob(os.path.join(DATA, "*.cpp")))] + \
           [(s, True) for s in sorted(glob.glob(os.path.join(FIXED, "*.cpp")))]


def compile_all(objdir):
    os.makedirs(objdir, exist_ok=True)
    gxx = toolchain.tool("g++")
    hdrs = glob.glob(os.path.join(DATA, "*.h")) + \
        glob.glob(os.path.join(DECOMP, "include", "**", "*.h"), recursive=True)
    # (and this script: the flags below)
    newest_hdr = max([os.path.getmtime(h) for h in hdrs] + [os.path.getmtime(__file__)])

    def cc(src_fixed):
        src, fixed = src_fixed
        obj = os.path.join(objdir, ("fixed_" if fixed else "") + os.path.basename(src)[:-4] + ".o")
        if not (os.path.exists(obj) and os.path.getmtime(obj) >= max(os.path.getmtime(src),
                                                                      newest_hdr)):
            # C++ (as Neversoft's sources were), for the game's struct types (decomp/include);
            # C++98: no narrowing errors in the word tables
            r = subprocess.run([gxx, "-m32", "-std=gnu++98", "-O0", "-fno-common", "-fno-ident",
                                "-fno-exceptions", "-w", "-funsigned-char", "-malign-data=abi",
                                "-g"] +
                               ([] if toolchain.WINDOWS else ["-fno-pic"]) +
                               ["-I", DATA] + INCLUDES + [
                                "-c", src, "-o", obj], capture_output=True, text=True)
            if r.returncode:
                raise SystemExit("data_image: %s\n%s" % (src, r.stderr[-4000:]))
        return obj

    with cf.ThreadPoolExecutor() as ex:
        return list(ex.map(cc, data_sources()))


FIXED_SEC = re.compile(r"\.psx(0x[0-9a-f]{8})$")
NATIVE_SEC = re.compile(r"\.data[$.]psx(0x[0-9a-f]{8})$")


def is_literal_section(name):
    return name.startswith(".rdata") or name.startswith(".rodata")


def cstring(data, off):
    end = data.index(b"\0", off)
    return data[off:end]


_var_sizes = {}


def var_sizes(objpath):
    """{name: size} of the variables an object defines, from its debug information."""
    if objpath not in _var_sizes:
        from layouts import Layouts, parse_dwarf, ref  # noqa: E402
        dump = subprocess.run([toolchain.tool("objdump"), "--dwarf=info", objpath],
                              capture_output=True, text=True).stdout
        dies = parse_dwarf(dump)
        lay = Layouts(dies)
        out = {}
        for d in dies.values():
            at = d["attrs"]
            if d["tag"] != "DW_TAG_variable" or "DW_AT_declaration" in at:
                continue
            # a C++ definition can be a specification of an earlier declaration
            spec = dies.get(ref(at.get("DW_AT_specification")), {"attrs": {}})["attrs"]
            typ = at.get("DW_AT_type") or spec.get("DW_AT_type")
            size = lay.size(lay.strip(ref(typ))) if typ else 0
            if not size:
                continue
            for k in ("DW_AT_name", "DW_AT_linkage_name"):
                for a in (at, spec):
                    if k in a:
                        out[a[k].split()[-1]] = size
        _var_sizes[objpath] = out
    return _var_sizes[objpath]


class Layout:
    """The decompiled data, laid out at the retail addresses.

    globals: [{addr, end, name, fixed, data, ptrs, lits}] sorted by address. `data` has every
      pointer resolved to the PS1 address of its target, listed in `ptrs` as (offset, symbol
      address, addend); a pointer to a string literal (linked natively, wherever it lands) is 0
      there and listed in `lits` as (offset, text).
    image: the fixed globals at their addresses (what the port copies to 0x80010000).
    literals: the text of every string literal in the native data.
    """


def build(objdir):
    objpaths = compile_all(objdir)
    srcs = data_sources()
    objs = [read_object(o) for o in objpaths]
    secaddr = []         # per object: section index -> PS1 address
    defined = {}         # global name -> (PS1 address, fixed)
    for (path, fixed), o in zip(srcs, objs):
        sa = {}
        for k, s in enumerate(o.sections):
            m = (FIXED_SEC if fixed else NATIVE_SEC).match(s["name"])
            if s["name"].startswith(".debug"):
                continue
            if m:
                a = int(m.group(1), 16)
                sa[k + 1] = a
                if not (BASE <= a and a + len(s["data"]) <= END):
                    raise SystemExit("data_image: %s: %s outside the image" % (path, s["name"]))
            elif FIXED_SEC.match(s["name"]) or NATIVE_SEC.match(s["name"]):
                raise SystemExit("data_image: %s: %s global in a %s file (PSX_%s)" % (
                    path, "fixed" if not fixed else "native", "native" if not fixed else "fixed",
                    "DATA" if fixed else "FIXED"))
            elif fixed and s["data"] and (any(s["data"]) or s["relocs"]):
                raise SystemExit("data_image: %s: data outside PSX_FIXED (section %s)" % (
                    path, s["name"]))
            elif not fixed and s["data"] and (any(s["data"]) or s["relocs"]) and \
                    not is_literal_section(s["name"]):
                raise SystemExit("data_image: %s: data outside PSX_DATA (section %s)" % (
                    path, s["name"]))
        for sym in o.symbols:
            if sym and sym["sec"] in sa and sym["cls"] == 2:  # external definition
                n = c_name(sym["name"])
                a = sa[sym["sec"]] + sym["value"]
                if n in defined and defined[n][0] != a:
                    raise SystemExit("data_image: %s defined twice" % n)
                defined[n] = (a, fixed)
        secaddr.append(sa)

    names = None
    placed = []          # [addr, end, name, data, fixed, ptrs, lits]
    literals = set()
    for (path, fixed), objpath, o, sa in zip(srcs, objpaths, objs, secaddr):
        for k, s in enumerate(o.sections):
            if not fixed and is_literal_section(s["name"]):
                literals.update(s["data"].split(b"\0")[:-1])
            if k + 1 not in sa:
                continue
            a = sa[k + 1]
            data = bytearray(s["data"])
            ptrs, lits = [], []
            for off, si, typ in s["relocs"]:
                sym = o.symbols[si]
                if typ != 6:
                    raise SystemExit("data_image: %s: relocation type %d" % (path, typ))
                v = struct.unpack_from("<I", data, off)[0]
                if sym["sec"] > 0 and sym["sec"] not in sa:
                    tsec = o.sections[sym["sec"] - 1]
                    if fixed or not is_literal_section(tsec["name"]):
                        raise SystemExit("data_image: %s: pointer to %s, not a %s" % (
                            path, sym["name"], "fixed global" if fixed else "global or literal"))
                    lits.append((off, cstring(tsec["data"], sym["value"] + v)))
                    struct.pack_into("<I", data, off, 0)
                    continue
                if sym["sec"] > 0:
                    t = sa[sym["sec"]] + sym["value"]
                else:
                    n = c_name(sym["name"])
                    if n in defined:
                        t, tfixed = defined[n]
                        if fixed and not tfixed:
                            raise SystemExit("data_image: %s: fixed data points at %s, which "
                                             "isn't fixed (decomp/data/fixed)" % (path, n))
                    else:
                        if names is None:
                            names = name_table()
                        cand = names.get(n, set())
                        m = ADDR_SUFFIX.search(n)
                        if len(cand) == 1:
                            t = next(iter(cand))
                        elif m:
                            t = int(m.group(1), 16)
                        else:
                            raise SystemExit("data_image: %s: no address for %s (%s)" % (
                                path, n, "ambiguous" if cand else "unknown"))
                ptrs.append((off, t, v))
                struct.pack_into("<I", data, off, (v + t) & 0xFFFFFFFF)
            gsym = next((sym["name"] for sym in o.symbols
                         if sym and sym["sec"] == k + 1 and sym["cls"] == 2), None)
            if gsym is None:
                raise SystemExit("data_image: %s: %s defines no global" % (path, s["name"]))
            # a section is padded (to 4 bytes): the global is as big as its type
            size = var_sizes(objpath).get(c_name(gsym))
            if size is not None and size < len(data):
                if any(data[size:]) or any(off + 4 > size for off, *_ in ptrs + lits):
                    raise SystemExit("data_image: %s: %s is %d bytes, its section %d" % (
                        path, gsym, size, len(data)))
                data = data[:size]
            placed.append([a, a + len(data), c_name(gsym), bytes(data), fixed, ptrs, lits])
    # gcc pads a section to its alignment (32 bytes for big arrays): a global ends where the next
    # one starts, if what's cut off is padding
    placed.sort()
    for p, q in zip(placed, placed[1:]):
        if q[0] < p[1]:
            cut = p[3][q[0] - p[0]:]
            if any(cut) or any(off >= q[0] - p[0] for off, *_ in p[5] + p[6]):
                raise SystemExit("data_image: %s (0x%08x-0x%08x) overlaps %s (0x%08x)" % (
                    p[2], p[0], p[1], q[2], q[0]))
            p[1], p[3] = q[0], p[3][:q[0] - p[0]]
    lay = Layout()
    lay.globals = [{"addr": a, "end": e, "name": n, "data": d, "fixed": f, "ptrs": pt, "lits": li}
                   for a, e, n, d, f, pt, li in placed]
    lay.image = bytearray(END - BASE)
    for g in lay.globals:
        if g["fixed"]:
            lay.image[g["addr"] - BASE:g["end"] - BASE] = g["data"]
    lay.literals = literals
    return lay


CHUNK_GAP = 0x1000  # native globals closer than this share a block (the gap kept, zero)


def native_blocks(lay):
    """The native globals in blocks with their retail layout: [(start, end, [global])]. The
    decompiled code relies on that layout (a struct the code names as several globals, an index
    from one table into the next), so the port keeps it rather than linking each global alone."""
    blocks = []
    for g in lay.globals:
        if g["fixed"]:
            continue
        if blocks and g["addr"] - blocks[-1][1] < CHUNK_GAP:
            blocks[-1][1] = g["end"]
            blocks[-1][2].append(g)
        else:
            blocks.append([g["addr"], g["end"], [g]])
    return blocks


def native_asm(lay, out_s):
    """The native data as assembly (out_s, with the raw bytes in out_s[:-2] + ".bin"): each block
    is a run of bytes at label pc_data_<start>, aligned as at its retail address; pointers are
    `psx_<addr> + addend` (resolved by the link's psx_syms.ld: the native block, a function's
    trampoline, PS1 RAM); the string literals follow, in .data too. -> (blocks [start, end], the
    PS1 addresses the pointers name)."""
    us = toolchain.USCORE
    blocks = native_blocks(lay)
    blob = bytearray()
    asm = ["\t.data"]
    strs = {}
    refs = set()
    binname = os.path.basename(out_s)[:-2] + ".bin"
    for start, end, gs in blocks:
        img = bytearray(end - start)
        words = {}
        for g in gs:
            img[g["addr"] - start:g["end"] - start] = g["data"]
            for off, t, v in g["ptrs"]:
                refs.add(t)
                words[g["addr"] - start + off] = "%spsx_%08x%s" % (
                    us, t, " + %d" % (v - (v >> 31 << 32)) if v else "")
            for off, text in g["lits"]:
                if text not in strs:
                    strs[text] = ".Lpcs%d" % len(strs)
                words[g["addr"] - start + off] = strs[text]
        asm += ["\t.p2align 4", "\t.skip %d" % (start & 15),
                "\t.globl %spc_data_%08x" % (us, start), "%spc_data_%08x:" % (us, start)]
        pos = 0
        for w in sorted(words):
            if w > pos:
                asm.append('\t.incbin "%s", %d, %d' % (binname, len(blob) + pos, w - pos))
            asm.append("\t.long %s" % words[w])
            pos = w + 4
        if pos < len(img):
            asm.append('\t.incbin "%s", %d, %d' % (binname, len(blob) + pos, len(img) - pos))
        blob += img
    # the tables' strings: writable, as all of the PS1's data was
    for text, label in strs.items():
        asm.append("%s:\n\t.byte %s" % (label, ", ".join(str(b) for b in text + b"\0")))
    if not toolchain.WINDOWS:
        asm.append("\t.section .note.GNU-stack,\"\",@progbits")
    text = "\n".join(asm) + "\n"
    binpath = os.path.join(os.path.dirname(out_s), binname)
    if not os.path.exists(binpath) or open(binpath, "rb").read() != blob:
        open(binpath, "wb").write(blob)
    if not os.path.exists(out_s) or open(out_s).read() != text:
        open(out_s, "w").write(text)
    return [[s, e] for s, e, _ in blocks], sorted(refs)


def retail_reference(exe_path):
    """The retail image as the port should have it: code cleared, the omitted data zeroed."""
    sys.path.insert(0, HERE)
    import embed_image  # noqa: E402
    d = open(exe_path, "rb").read()
    base, size = struct.unpack_from("<2I", d, 0x18)
    ref = bytearray(END - BASE)
    ref[:size] = d[0x800:0x800 + size]
    for a in embed_image.code_words():
        ref[a - BASE:a - BASE + 4] = bytes(4)
    for line in open(os.path.join(DATA, "omitted.tsv"), encoding="utf-8"):
        if line.startswith("0x"):
            s, e = (int(x, 16) for x in line.split("\t")[:2])
            ref[s - BASE:e - BASE] = bytes(e - s)
    ref[0x800D21C8 - BASE:0x800DC94C - BASE] = bytes(0x800DC94C - 0x800D21C8)  # .bss
    return ref


def unused_but_named():
    """Globals the decompiled code names that omitted.tsv calls unused (library data, jump
    tables): [(tu, name, addr)]."""
    om = []
    for line in open(os.path.join(DATA, "omitted.tsv"), encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if p[0].startswith("0x") and not p[3].startswith("memory card icon"):
            om.append((int(p[0], 16), int(p[1], 16)))
    out = []
    for f in glob.glob(os.path.join(DECOMP, "src", "*", "*.syms.json")):
        src = ""
        for ext in (".c", ".cpp"):
            if os.path.exists(f[:-len(".syms.json")] + ext):
                src = open(f[:-len(".syms.json")] + ext, encoding="utf-8", errors="replace").read()
        for n, a in json.load(open(f)).items():
            # the matching build's own anchors (its switch tables: .rdata, DAT_) aren't uses unless
            # the source names them
            if n != "gp" and any(s <= int(a, 16) < e for s, e in om) and \
                    re.search(r"\b%s\b" % re.escape(n), src):
                out.append((os.path.relpath(f, DECOMP), n, a))
    return out


C_TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"((?:\\.|[^\\"])*)"', re.S)
C_ESCAPES = {"n": 10, "t": 9, "r": 13, "0": 0, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92,
             "'": 39, '"': 34, "?": 63}


def c_literal_bytes(body):
    out, i = bytearray(), 0
    while i < len(body):
        c = body[i]
        if c != "\\":
            out += c.encode("utf-8")
            i += 1
            continue
        e = body[i + 1]
        if e == "x":
            m = re.match(r"[0-9a-fA-F]+", body[i + 2:])
            out.append(int(m.group(0), 16) & 0xFF)
            i += 2 + len(m.group(0))
        elif e in "01234567":
            m = re.match(r"[0-7]{1,3}", body[i + 1:])
            out.append(int(m.group(0), 8) & 0xFF)
            i += 1 + len(m.group(0))
        else:
            out.append(C_ESCAPES.get(e, ord(e)))
            i += 2
    return bytes(out)


def source_literals():
    """The text of every string literal in the decompiled code (adjacent literals joined)."""
    out = set()
    for f in glob.glob(os.path.join(DECOMP, "src", "**", "*.c*"), recursive=True):
        if not f.endswith((".c", ".cpp")):
            continue
        text = open(f, encoding="utf-8", errors="replace").read()
        run, last_end = None, -1
        for m in C_TOKEN.finditer(text):
            if m.group(1) is not None:
                b = c_literal_bytes(m.group(1))
                if run is not None and not text[last_end:m.start()].strip():
                    run += b
                else:
                    if run is not None:
                        out.add(run)
                    run = b
                last_end = m.end()
        if run is not None:
            out.add(run)
    return out


def check(lay, exe_path):
    """The decompiled data against the retail executable's: every global (native or fixed) laid
    out at its retail address must have the retail bytes, a pointer to a literal must point at
    the same text in retail, and retail data no global holds any more must be text the source
    has as a literal (the code's and the tables' strings)."""
    named = unused_but_named()
    for tu, n, a in named:
        print("data_image: %s uses %s (%s), which omitted.tsv leaves out" % (tu, n, a))
    ref = retail_reference(exe_path)
    virt = bytearray(END - BASE)
    covered = bytearray(END - BASE)
    bad = {}
    for g in lay.globals:
        a, e = g["addr"], g["end"]
        virt[a - BASE:e - BASE] = g["data"]
        covered[a - BASE:e - BASE] = b"\1" * (e - a)
        for off, text in g["lits"]:
            w = struct.unpack_from("<I", ref, a - BASE + off)[0]
            if not (BASE <= w < END) or bytes(ref[w - BASE:w - BASE + len(text) + 1]) != text + b"\0":
                bad.setdefault(g["name"], []).append(a + off)
            else:
                virt[a + off - BASE:a + off - BASE + 4] = ref[a + off - BASE:a + off - BASE + 4]
    keys = [g["addr"] for g in lay.globals]
    known = None
    i, n = 0, len(virt)
    while i < n:
        if virt[i] == ref[i]:
            i += 1
            continue
        a = BASE + i
        if covered[i]:
            k = bisect.bisect_right(keys, a) - 1
            bad.setdefault(lay.globals[k]["name"], []).append(a)
            i += 1
            continue
        # retail bytes no global holds: a string (from its start) that the source has as text
        s = i
        while s > 0 and not covered[s - 1] and ref[s - 1] != 0:
            s -= 1
        e = ref.index(b"\0", i) if b"\0" in ref[i:i + 4096] else i + 1
        if known is None:
            known = lay.literals | source_literals()
        if bytes(ref[s:e]) not in known or any(covered[s:e]):
            bad.setdefault("(no global) 0x%08x" % (BASE + s), []).append(a)
        i = e + 1
    if not bad:
        if named:
            return 1
        print("data_image: the same as the retail data (code and omitted.tsv aside)")
        return 0
    for who, addrs in list(bad.items())[:40]:
        a = addrs[0]
        print("data_image: %s differs at 0x%08x (%d bytes): built %s, retail %s" % (
            who, a, len(addrs), virt[a - BASE:a - BASE + 8].hex(), ref[a - BASE:a - BASE + 8].hex()))
    print("data_image: %d globals differ from the retail data" % len(bad))
    return 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--check", metavar="SLUS_008.60")
    ap.add_argument("--objdir", default=OBJDIR)
    ap.add_argument("--asm", help="write the native data as assembly (native_asm) and, next to "
                    "it, <name>.json: its blocks, the fixed globals and what its pointers name")
    a = ap.parse_args()
    toolchain.add_to_path()
    lay = build(a.objdir)
    with open(a.out + ".tmp", "wb") as f:
        f.write(lay.image)
    os.replace(a.out + ".tmp", a.out)
    fixed = [g for g in lay.globals if g["fixed"]]
    print("data_image: %d globals (%d fixed in the image), %d bytes of data, %d literals" % (
        len(lay.globals), len(fixed), sum(g["end"] - g["addr"] for g in lay.globals),
        len(lay.literals)))
    if a.asm:
        blocks, refs = native_asm(lay, a.asm)
        man = {"blocks": blocks, "fixed": [[g["addr"], g["end"]] for g in fixed], "refs": refs}
        text = json.dumps(man, indent=0)
        mpath = a.asm[:-2] + ".json"
        if not os.path.exists(mpath) or open(mpath).read() != text:
            open(mpath, "w").write(text)
    if a.check:
        return check(lay, a.check)
    return 0


if __name__ == "__main__":
    sys.exit(main())
