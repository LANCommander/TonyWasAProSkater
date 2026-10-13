#!/usr/bin/env python3
"""The port's data image from the decompiled data (decomp/data/*.c): build/pc/gen/image.bin.

The game's initialised globals live at their retail addresses in PS1 RAM (0x80010000 on), so the
port copies this image there at startup. Each decomp/data file is compiled natively (gcc -m32;
PSX_DATA gives every global a section named after its address); this places each section at its
address and resolves every pointer (an i386 DIR32 relocation) to the PS1 address of its target:
a global in these files, else a name from functions.tsv, the TUs' .syms.json or data_names.tsv,
else a `_<8 hex digits>` suffix. Everything else in the image (code, .bss) is zero.

    python platform/pc/tools/data_image.py <out.bin> [--check SLUS_008.60]

--check compares the image with the retail executable's: equal except the code (which the port
never has) and the data decomp/data/omitted.tsv lists. Prints the globals that differ.
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
sys.path.insert(0, HERE)
import toolchain  # noqa: E402

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


def compile_all(objdir):
    os.makedirs(objdir, exist_ok=True)
    gcc = toolchain.tool("gcc")
    srcs = sorted(glob.glob(os.path.join(DATA, "*.c")))
    hdr = os.path.join(DATA, "psxdata.h")

    def cc(src):
        obj = os.path.join(objdir, os.path.basename(src)[:-2] + ".o")
        if not (os.path.exists(obj) and os.path.getmtime(obj) >= max(os.path.getmtime(src),
                                                                      os.path.getmtime(hdr))):
            r = subprocess.run([gcc, "-m32", "-std=gnu11", "-O0", "-fno-common", "-fno-ident", "-w"] +
                               ([] if toolchain.WINDOWS else ["-fno-pic"]) +
                               ["-I", DATA, "-c", src, "-o", obj], capture_output=True, text=True)
            if r.returncode:
                raise SystemExit("data_image: %s\n%s" % (src, r.stderr[-4000:]))
        return obj

    with cf.ThreadPoolExecutor() as ex:
        return list(ex.map(cc, srcs))


def build(objdir):
    """(image bytes, [(addr, end, name)] of every global placed)."""
    objs = [read_object(o) for o in compile_all(objdir)]
    img = bytearray(END - BASE)
    placed = []          # (addr, end, name)
    secaddr = []         # per object: section index -> PS1 address
    defined = {}         # global name -> PS1 address
    for path, o in zip(sorted(glob.glob(os.path.join(DATA, "*.c"))), objs):
        sa = {}
        for k, s in enumerate(o.sections):
            if s["name"].startswith(".psx0x"):
                a = int(s["name"][4:], 16)
                sa[k + 1] = a
                if not (BASE <= a and a + len(s["data"]) <= END):
                    raise SystemExit("data_image: %s: %s outside the image" % (path, s["name"]))
            elif s["data"] and any(s["data"]) or s["relocs"]:
                raise SystemExit("data_image: %s: data outside PSX_DATA (section %s)" % (
                    path, s["name"]))
        for sym in o.symbols:
            if sym and sym["sec"] in sa and sym["cls"] == 2:  # external definition
                n = c_name(sym["name"])
                a = sa[sym["sec"]] + sym["value"]
                if n in defined and defined[n] != a:
                    raise SystemExit("data_image: %s defined twice" % n)
                defined[n] = a
        secaddr.append(sa)

    names = None
    for path, o, sa in zip(sorted(glob.glob(os.path.join(DATA, "*.c"))), objs, secaddr):
        for k, s in enumerate(o.sections):
            if k + 1 not in sa:
                continue
            a = sa[k + 1]
            data = bytearray(s["data"])
            for off, si, typ in s["relocs"]:
                sym = o.symbols[si]
                if typ != 6:
                    raise SystemExit("data_image: %s: relocation type %d" % (path, typ))
                if sym["sec"] > 0:
                    if sym["sec"] not in sa:
                        raise SystemExit("data_image: %s: pointer to %s, not PSX_DATA" % (
                            path, sym["name"]))
                    t = sa[sym["sec"]] + sym["value"]
                else:
                    n = c_name(sym["name"])
                    if n in defined:
                        t = defined[n]
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
                v = struct.unpack_from("<I", data, off)[0]
                struct.pack_into("<I", data, off, (v + t) & 0xFFFFFFFF)
            name = next((sym["name"].lstrip("_") for sym in o.symbols
                         if sym and sym["sec"] == k + 1 and sym["cls"] == 2), s["name"])
            placed.append([a, a + len(data), name, bytes(data)])
    # gcc pads a section to its alignment (32 bytes for big arrays): a global ends where the next
    # one starts, if what's cut off is padding
    placed.sort()
    for p, q in zip(placed, placed[1:]):
        if q[0] < p[1]:
            cut = p[3][q[0] - p[0]:]
            if any(cut):
                raise SystemExit("data_image: %s (0x%08x-0x%08x) overlaps %s (0x%08x)" % (
                    p[2], p[0], p[1], q[2], q[0]))
            p[1], p[3] = q[0], p[3][:q[0] - p[0]]
    for a, e, name, data in placed:
        img[a - BASE:e - BASE] = data
    placed = [(a, e, name) for a, e, name, _ in placed]
    return img, placed


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


def check(img, placed, exe_path):
    named = unused_but_named()
    for tu, n, a in named:
        print("data_image: %s uses %s (%s), which omitted.tsv leaves out" % (tu, n, a))
    ref = retail_reference(exe_path)
    if img == ref:
        if named:
            return 1
        print("data_image: the same as the retail data (code and omitted.tsv aside)")
        return 0
    keys = [p[0] for p in placed]
    bad = {}
    for i in range(len(img)):
        if img[i] != ref[i]:
            a = BASE + i
            k = bisect.bisect_right(keys, a) - 1
            who = placed[k][2] if k >= 0 and a < placed[k][1] else "(no global) 0x%08x" % (a & ~3)
            bad.setdefault(who, []).append(a)
    for who, addrs in list(bad.items())[:40]:
        a = addrs[0]
        print("data_image: %s differs at 0x%08x (%d bytes): built %s, retail %s" % (
            who, a, len(addrs), img[a - BASE:a - BASE + 8].hex(), ref[a - BASE:a - BASE + 8].hex()))
    print("data_image: %d globals differ from the retail data" % len(bad))
    return 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--check", metavar="SLUS_008.60")
    ap.add_argument("--objdir", default=os.path.join(toolchain.OUT, "data"))
    a = ap.parse_args()
    toolchain.add_to_path()
    img, placed = build(a.objdir)
    with open(a.out + ".tmp", "wb") as f:
        f.write(img)
    os.replace(a.out + ".tmp", a.out)
    print("data_image: %d globals, %d bytes of data" % (
        len(placed), sum(e - s for s, e, _ in placed)))
    if a.check:
        return check(img, placed, a.check)
    return 0


if __name__ == "__main__":
    sys.exit(main())
