#!/usr/bin/env python3
"""Build the native Windows and Linux port of THPS1 (see platform/pc/README.md).

    python platform/pc/tools/build.py [-j N] [--clean] [--only SUBSTR] [--no-link] [--release]
                                      [--version X.Y.Z]

The game's data (game/data: ordinary C++ globals) is compiled first; its references to functions
are renamed to `psx_<addr>` as the code's are (below). Then, for every game TU (each .c/.cpp
under game/src, a retail module; its functions are its header comments, HEADER_RE):

  1. source transforms (portsrc.py): inline GTE asm -> software GTE calls; for C++, the egcs
     virtual-function ABI on top of non-virtual native classes (vtables: game/data by name,
     game/vtables.tsv);
  2. compile to assembly (cdecl everywhere on i386: -mabi=sysv);
  3. assemble, then rename the function symbols: a call resolves to `psx_<addr>` (the function's
     retail address, from functions.tsv or the TU's .syms.json), and each of the TU's own
     functions becomes `pcfn_<addr>`. Globals keep their C names: game/data defines them.

Then it generates the glue (linker script naming every `psx_<addr>` function's code, virtual-
dispatch thunks, stubs for anything not implemented yet) and links everything with the runtime
into build/pc/thps.exe (Linux: build/pc-linux/thps; see toolchain.py).
"""
import argparse
import concurrent.futures as cf
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PC = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(PC))
DECOMP = os.path.join(ROOT, "decomp")  # the frozen matching decompilation (its tools only)
GAME = os.path.join(ROOT, "game")      # the port's game code, data and headers
sys.path.insert(0, HERE)
import portsrc  # noqa: E402
import toolchain  # noqa: E402
from toolchain import MFLAG, OUT, SANITIZE, TOOLBIN, USCORE, WINDOWS, X64, tool  # noqa: E402,F401

GAME_CODE_END = 0x800858AC
CODE_END = 0x8009D0E0  # hand-written assembly rewritten in C (reimpl.tsv) lies past the game code

COMMON_FLAGS = [
    MFLAG, "-mno-ms-bitfields", "-O1", "-g",
    "-fno-strict-aliasing", "-fwrapv", "-fno-delete-null-pointer-checks", "-fno-builtin",
    "-fno-omit-frame-pointer", "-w", "-DPC_PORT=1",
    # outgoing args are stored, never pushed, so the only post-call %esp adjustment is the
    # aggregate-return one caller_pops_sret removes
    "-maccumulate-outgoing-args",
    # plain char is unsigned in the PS1 ABI (egcs on MIPS): the decompiled code is verified with it
    "-funsigned-char",
    "-I", os.path.join(PC, "include", "psyq"), "-I", os.path.join(PC, "include"),
    "-I", os.path.join(GAME, "include"),
    "-include", os.path.join(PC, "include", "pc_game.h"),
] + SANITIZE
if not X64:
    # i386: cdecl for C++ members too (no thiscall: C code calls them with `this` first), 8-byte
    # aligned doubles as everywhere else
    COMMON_FLAGS += ["-mabi=sysv", "-malign-double"]
if not USCORE:
    # x64 PE and ELF don't prefix C names with '_': have the compiler do it anyway, so the symbol
    # pipeline below is the same everywhere (build_tu takes the '_' off again at the end)
    COMMON_FLAGS += ["-fleading-underscore"]
if X64 and WINDOWS:
    # direct (RIP-relative) references: MinGW's default medium model reaches globals through
    # `refptr.` pointers, which also lose -fleading-underscore's prefix
    COMMON_FLAGS += ["-mcmodel=small"]
if not WINDOWS:
    # ELF: no position independence, and (i386) small structs returned in registers as on Windows
    # (the runtime is built the same way)
    COMMON_FLAGS += ["-fno-pic", "-fno-pie"] + ([] if X64 else ["-freg-struct-return"])
C_FLAGS = ["-std=gnu89", "-fpermissive", "-fcommon"]
# -fno-lifetime-dse: egcs destructors re-store their class's vptr (portsrc adds the store); g++ would
# drop stores into an object whose lifetime is ending, and those vptrs are read by the base dtors
CXX_FLAGS = ["-std=gnu++98", "-fpermissive", "-fno-exceptions", "-fno-rtti",
             "-fno-threadsafe-statics", "-fno-use-cxa-atexit", "-fno-lifetime-dse"]


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    return r.returncode, r.stdout, r.stderr


# --------------------------------------------------------------------------------------------
# inputs


def read_tsv(path):
    for line in open(path, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        yield line.rstrip("\n").split("\t")


def load_functions():
    """addr -> (size, mangled, demangled) for game functions (decomp/functions.tsv)."""
    fn = {}
    for p in read_tsv(os.path.join(GAME, "functions.tsv")):
        if not p[0].startswith("0x"):
            continue
        fn[int(p[0], 16)] = (int(p[1]), p[2], p[4] if len(p) > 4 else p[2])
    return fn


# A function's header comment, at the start of a line just above it: `/* <name> (0x<retail address>)
# [anything] */`, name as the retail symbol (GNU v2 mangling for C++). Prose that only mentions an
# address, `/* Label (0x80087b3c): ...`, is not one.
HEADER_RE = re.compile(r"^/\*[ \t]*(\S+)[ \t]+\((0x[0-9a-fA-F]{8})\)(?![:,])", re.M)
SRC_DIR = os.path.join(GAME, "src")


def game_sources():
    """Every game TU: the .c/.cpp files under game/src, as paths relative to game/."""
    out = []
    for dirpath, _, files in os.walk(SRC_DIR):
        for f in files:
            if f.endswith((".c", ".cpp")):
                out.append(os.path.relpath(os.path.join(dirpath, f), GAME).replace("\\", "/"))
    return sorted(out)


def load_tus():
    """{src: TU}. A TU's functions are its header comments: [(addr, retail name)], in file order;
    `addr`/`name` are the first one's (the file's primary function)."""
    tus = {}
    for src in game_sources():
        path = os.path.join(GAME, src)
        funcs = [(int(m.group(2), 16), m.group(1))
                 for m in HEADER_RE.finditer(open(path, encoding="utf-8", errors="replace").read())]
        if not funcs:
            raise SystemExit("%s: no function header (/* <name> (0x<addr>) */)" % src)
        sj = os.path.splitext(path)[0] + ".syms.json"
        tus[src] = {"src": src, "funcs": funcs, "addr": funcs[0][0], "name": funcs[0][1],
                    "syms": json.load(open(sj)) if os.path.exists(sj) else {}}
    return tus


def load_vtable_overrides():
    """{tu src: {class: vtable}} (game/vtables.tsv)."""
    out = {}
    for p in read_tsv(os.path.join(GAME, "vtables.tsv")):
        out.setdefault(p[0], {})[p[1]] = p[2]
    return out


# --------------------------------------------------------------------------------------------
# demangling


def demangle_gnu(names):
    sys.path.insert(0, os.path.join(DECOMP, "tools"))
    from demangle import demangle  # noqa: E402
    return demangle(names)


def demangle_itanium(names):
    names = list(dict.fromkeys(names))
    if not names:
        return {}
    out = subprocess.run([tool("c++filt"), "-n"], input="\n".join(names) + "\n",
                         capture_output=True, text=True).stdout.split("\n")
    return dict(zip(names, (o.strip() for o in out)))


def norm_sig(d):
    """Normalise a demangled signature so GNU-v2 and Itanium spellings compare equal."""
    d = d.replace("(void)", "()")
    d = re.sub(r"\s+", "", d)
    d = d.replace("constchar", "charconst").replace("constCVector", "CVectorconst")
    d = re.sub(r"\bconst(\w+)&", r"\1const&", d)
    # operator new takes a size_t: 64 bits in an x64 build, as retail's unsigned int
    d = re.sub(r"(operatornew(?:\[\])?)\(unsignedlong(?:long)?\)", r"\1(unsignedint)", d)
    # the port's 32-bit words are ints where retail had longs (an LP64 long is 64 bits)
    d = re.sub(r"(?<![A-Za-z0-9_])(unsigned|signed)?long(?!long|double)(?![A-Za-z0-9_])",
               lambda m: (m.group(1) or "") + "int", d)
    return d


def qual_name(d):
    i = d.find("(")
    return re.sub(r"\s+", "", d if i < 0 else d[:i])


# --------------------------------------------------------------------------------------------
# the game's data


DATA_DIR = os.path.join(GAME, "data")
# C++ (as Neversoft's sources were), for the game's struct types (game/include, with full debug
# types: GAME_DATA_TU); C++98: no narrowing errors in the word tables
DATA_FLAGS = [MFLAG, "-std=gnu++98", "-O1", "-g", "-fno-common", "-fno-exceptions", "-fno-rtti",
              "-w", "-funsigned-char", "-DPC_PORT=1", "-DGAME_DATA_TU", "-I", DATA_DIR,
              "-I", os.path.join(GAME, "include"), "-I", os.path.join(PC, "include", "psyq"),
              "-I", os.path.join(PC, "include")] + ([] if WINDOWS else ["-fno-pic"]) + SANITIZE


def nm_symbols(obj):
    """([defined global names], [undefined names]) of an object, as the assembler spells them."""
    rc, so, se = run([tool("nm"), obj])
    if rc:
        raise SystemExit("nm %s: %s" % (obj, se))
    defined, undefined = [], []
    for line in so.splitlines():
        p = line.split()
        if len(p) == 2 and p[0] == "U":
            undefined.append(p[1])
        elif len(p) == 3 and p[1] in "DBRCVW":
            defined.append(p[2])
    return defined, undefined


def build_data(ctx):
    """Compile game/data/*.cpp into OUT/dataobj: ordinary globals, linked by their C names. A pointer
    to a function becomes `psx_<addr>`, as the TUs' calls do. Returns {objs, defined (C names),
    psx_syms (as PE spells them: `_psx_80012345`)}."""
    # (not OUT/data: on Windows that is the game's Data folder)
    out = os.path.join(OUT, "dataobj")
    os.makedirs(out, exist_ok=True)
    srcs = sorted(glob.glob(os.path.join(DATA_DIR, "*.cpp")))
    hdrs = glob.glob(os.path.join(DATA_DIR, "*.h")) + \
        glob.glob(os.path.join(GAME, "include", "**", "*.h"), recursive=True)
    newest = max([os.path.getmtime(h) for h in hdrs] + [os.path.getmtime(__file__)])

    def cc(src):
        raw = os.path.join(out, os.path.basename(src)[:-4] + ".raw.o")
        if not (os.path.exists(raw) and os.path.getmtime(raw) >= max(os.path.getmtime(src),
                                                                      newest)):
            rc, so, se = run([tool("g++")] + DATA_FLAGS + ["-c", src, "-o", raw])
            if rc:
                raise SystemExit("game/data: %s\n%s" % (src, se[-4000:]))
        return raw, nm_symbols(raw)

    with cf.ThreadPoolExecutor() as ex:
        raws = list(ex.map(cc, srcs))
    unprefix = (lambda s: s[len(USCORE):] if USCORE and s.startswith(USCORE) else s)
    defined = set()
    for raw, (d, u) in raws:
        for s in d:
            n = unprefix(s)
            if n in defined:
                raise SystemExit("game/data: %s defined twice" % n)
            defined.add(n)
    psx = set()

    def rename(item):
        raw, (d, u) = item
        renames = {}
        for s in u:
            n = unprefix(s)
            if n in defined:
                continue
            a = ctx.func_addr(n)
            if a is not None:
                r = ctx.native_at.get("_psx_%08x" % a, "_psx_%08x" % a)
                renames[s] = USCORE + r[1:]
                if r.startswith("_psx_"):
                    psx.add("_psx_%08x" % a)
        obj = raw[:-6] + ".o"
        rn = raw[:-6] + ".rename"
        open(rn, "w").write("".join("%s %s\n" % kv for kv in renames.items()))
        rc, so, se = run([tool("objcopy"), "--redefine-syms=" + rn, raw, obj])
        if rc:
            raise SystemExit("objcopy %s: %s" % (raw, se))
        return obj

    with cf.ThreadPoolExecutor() as ex:
        objs = list(ex.map(rename, raws))
    return {"objs": objs, "defined": defined, "psx_syms": sorted(psx)}


# --------------------------------------------------------------------------------------------
# per-TU build


class Ctx:
    """Read-only data every worker needs."""

    def __init__(self):
        self.functions = load_functions()
        self.fn_by_name = {}
        for a, (sz, mangled, dem) in self.functions.items():
            self.fn_by_name.setdefault(mangled, a)
        self.tus = load_tus()
        # hand-written assembly rewritten in C: callable by its label from other rewrites
        for tu in self.tus.values():
            for a, name in tu["funcs"]:
                if a >= GAME_CODE_END:
                    self.fn_by_name.setdefault(name, a)
        self.fn_by_sig = {}
        self.fn_by_qual = {}
        for a, (sz, mangled, dem) in self.functions.items():
            self.fn_by_sig.setdefault(norm_sig(dem), []).append(a)
            self.fn_by_qual.setdefault(qual_name(dem), []).append(a)
        # function name -> address, from every TU's syms.json (names with one address only)
        seen = {}
        for tu in self.tus.values():
            for k, v in tu["syms"].items():
                if k.startswith(".") or k == "gp" or int(v, 16) >= CODE_END:
                    continue
                seen.setdefault(k, set()).add(int(v, 16))
        self.global_syms = {k: next(iter(v)) for k, v in seen.items() if len(v) == 1}
        allnames = set()
        for tu in self.tus.values():
            allnames.update(k for k in tu["syms"] if not k.startswith("."))
        allnames.update(m for (_, m, _) in self.functions.values())
        allnames.update(n for tu in self.tus.values() for _, n in tu["funcs"])
        self.gnu_dem = demangle_gnu(sorted(allnames))
        img = open(os.path.join(ROOT, "extern", "thps1_retail", "SLUS_008.60"), "rb").read()
        self.image = img
        off = 0x800D101C - 0x80010000 + 0x800
        self.static_ctors = [int.from_bytes(img[off + 4 * i:off + 4 * i + 4], "little")
                             for i in range(19)]
        self.psyq = {}
        self.native_at = {}
        p = os.path.join(ROOT, "build", "psyq_names.tsv")
        if os.path.exists(p):
            for q in read_tsv(p):
                self.psyq.setdefault(q[1], int(q[0], 16))
                t = NATIVE_RENAME.get(q[1]) or NATIVE_AT.get(q[1])
                if t:  # every copy of a function linked more than once
                    self.native_at["_psx_%08x" % int(q[0], 16)] = "_" + t
        # the game's data, compiled: the globals the TUs reach by name
        self.data = build_data(self)
        self.data_defined = self.data["defined"]
        # every class's vtable: game/data's __vt_<len><class>, or the TU's own (vtables.tsv)
        self.vtables = {}
        for n in self.data_defined:
            m = re.match(r"^__vt_(\d+)(\w+)$", n)
            if m and len(m.group(2)) == int(m.group(1)):
                self.vtables[m.group(2)] = n
        self.vtable_overrides = load_vtable_overrides()

    def func_addr(self, name):
        """The retail address of a function the data names (functions.tsv, the TUs' syms.json, the
        Psy-Q names, or a FUN_<addr>-style suffix), else None."""
        a = self.fn_by_name.get(name, self.global_syms.get(name, self.psyq.get(name)))
        if a is None:
            m = re.search(r"_([0-9a-f]{8})$", name)
            a = int(m.group(1), 16) if m else None
        return a if a is not None and a < CODE_END else None


BUILTIN_CXX = {
    "operatornew(unsignedint)": "__builtin_new",
    "operatordelete(void*)": "__builtin_delete",
    "operatornew[](unsignedint)": "__builtin_vec_new",
    "operatordelete[](void*)": "__builtin_vec_delete",
}

# symbols the native toolchain provides; never remapped
NATIVE_OK = re.compile(r"^(__alloca|___chkstk.*|__[a-z]*div[sd]i3|__[a-z]*mod[sd]i3|__mul[sd]i3|"
                       r"__ashldi3|__ashrdi3|__lshrdi3|__cmpdi2|__ucmpdi2|_?memcpy|_?memset|"
                       r"_?memmove|__fixunsdfdi|__floatdidf|_?abort|_?pc_geom\w*|_?M3dMaths_LastVec|"
                       r"_?M3dAsm_ColourCode|__ZTV.*|___cxa_.*|"
                       r"___gxx_personality.*|__Unwind.*|___dso_handle|_atexit|"
                       r"__GLOBAL_OFFSET_TABLE_|_?pc_delete_mark|_?pc_vdelete|"
                       # the C library: the host's, not Psy-Q's LIBC at its retail addresses
                       r"_?sprintf|_?strcpy|_?strlen|_?strcmp|_?strncmp|_?memchr|_?malloc|_?free)$")

# Psy-Q functions whose callers get a native port function instead: the game's debug printf goes
# to the port's trace log, and libsn's operator delete (which freed into the BIOS heap, which the
# game never allocates from) does nothing
NATIVE_RENAME = {"printf": "PC_GameDebugf", "puts": "PC_GameDebugPuts",
                 "__builtin_delete": "pc_delete_unowned"}
# The same, by address only: libgte is src/pc_libgte.cpp (PC_<name>). Not by name: decompiled
# sources call the game's own M3dMaths_SquareRoot0 (0x8008b5dc) `SquareRoot0` too.
NATIVE_AT = dict((n, "PC_" + n) for n in (
    "SetRotMatrix", "SetTransMatrix", "SetGeomScreen", "SetGeomOffset", "SetFarColor", "InitGeom",
    "TransposeMatrix", "TransMatrix", "MulMatrix", "MulMatrix0", "ApplyMatrixSV", "ApplyMatrixLV",
    "RotMatrixYXZ", "RotMatrixYXZ_gte", "RotMatrixY", "VectorNormal", "VectorNormalSS",
    "SquareRoot0", "catan", "ratan2",
    # libgpu's pure helpers (src/pc_libgpu.cpp)
    "SetDefDrawEnv", "SetDefDispEnv", "SetDrawArea", "SetDrawTPage", "SetPolyF4", "GetTPage",
    "GetClut", "DrawSync"))


def strip_us(sym):
    return sym[1:] if sym.startswith("_") else sym


INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]+)"[^\n]*$', re.M)
HEADER_DIR = os.path.join(GAME, "include")


def expand_includes(text, path, seen=None):
    """Inline the project's own headers (#include "..." found next to the source or in
    decomp/include), each once per TU, so portsrc sees the classes they declare. #line markers keep
    compiler messages and debug info pointing at the real files. Returns (text, [header paths])."""
    seen = set() if seen is None else seen
    out, pos, line = [], 0, 1
    for m in INCLUDE_RE.finditer(text):
        name = m.group(1)
        cand = [os.path.join(os.path.dirname(path), name), os.path.join(HEADER_DIR, name)]
        hdr = next((os.path.normpath(c) for c in cand if os.path.isfile(c)), None)
        if hdr is None:
            continue
        out.append(text[pos:m.start()])
        line += text.count("\n", pos, m.end())  # now the #include's own line number
        pos = m.end()
        if hdr not in seen:
            seen.add(hdr)
            body, _ = expand_includes(open(hdr, encoding="utf-8").read(), hdr, seen)
            out.append('#line 1 "%s"\n%s\n#line %d "%s"' % (
                hdr.replace("\\", "/"), body, line + 1, path.replace("\\", "/")))
    out.append(text[pos:])
    return "".join(out), sorted(seen)


VDEF = "_pcvdef"


def itanium_member(sym):
    """('Class', 'method') for a mangled `[_]_ZN<len>Class<len>method...E...`, else None."""
    m = re.match(r"_?_ZN", sym)
    if not m:
        return None
    i, parts = m.end(), []
    while i < len(sym) and sym[i].isdigit():
        j = i
        while sym[j].isdigit():
            j += 1
        n = int(sym[i:j])
        parts.append(sym[j:j + n])
        i = j + n
    return (parts[-2], parts[-1]) if len(parts) >= 2 and i < len(sym) and sym[i] == "E" else None


def param_count(d):
    """The number of parameters in a demangled signature."""
    inner = d[d.find("(") + 1:d.rfind(")")].strip()
    if not inner or inner == "void":
        return 0
    return len(portsrc._split_top(inner, ","))


def is_virtual(classes, cls, meth, d):
    """Whether Class::meth with demangled signature d is one of the class's virtual methods (not
    a non-virtual overload of the same name: CTape::AI(int) next to the virtual AI())."""
    c = classes.get(cls)
    if not c or meth not in c["slots"]:
        return False
    counts = c.get("vparams", {}).get(meth)
    return meth == "~" or not counts or param_count(d) in counts


def split_virtual_defs(asm, classes):
    """A TU that defines a virtual member function may also call it virtually (on another object):
    g++ binds such a call to the local definition, skipping the override the vtable would pick.
    Rename the definition (label, .globl, debug info) to <name>_pcvdef so the calls stay external
    and resolve to the virtual-dispatch thunk; plan() maps the renamed definition back."""
    for name in set(re.findall(r"^(_?_ZN\w+):", asm, re.M)):
        cm = itanium_member(name)
        if not cm or cm[0] not in classes or cm[1] not in classes[cm[0]]["slots"]:
            continue
        if not re.search(r"^\s*(call|jmp)\s+%s\b" % re.escape(name), asm, re.M):
            continue
        bare = name[1:] if name.startswith("__") else name
        if not is_virtual(classes, cm[0], cm[1], demangle_itanium([bare]).get(bare, bare)):
            continue
        asm = re.sub(r"\b%s\b" % re.escape(name), name + VDEF, asm)
        asm = re.sub(r"^(\s*(?:call|jmp)\s+)%s\b" % re.escape(name + VDEF), r"\g<1>" + name, asm,
                     flags=re.M)
    return asm


def tukey(tu):
    return re.sub(r"\W", "_", os.path.splitext(tu["src"])[0])


def tu_paths(tu):
    base = os.path.join(OUT, "obj", os.path.splitext(tu["src"])[0])
    return base


def build_tu(ctx, tu, flags_hash):
    src_path = os.path.join(GAME, tu["src"])
    base = tu_paths(tu)
    os.makedirs(os.path.dirname(base), exist_ok=True)
    is_cxx = tu["src"].endswith(".cpp")
    obj = base + ".o"
    meta_path = base + ".json"
    text, headers = expand_includes(open(src_path, encoding="utf-8", errors="replace").read(),
                                    src_path)
    vtables = dict(ctx.vtables)
    vtables.update(ctx.vtable_overrides.get(tu["src"], {}))
    stamp = hashlib.sha1((text + json.dumps(tu["syms"], sort_keys=True) +
                          json.dumps(vtables, sort_keys=True) + flags_hash).encode()).hexdigest()
    if os.path.exists(obj) and os.path.exists(meta_path):
        meta = json.load(open(meta_path))
        if meta.get("stamp") == stamp:
            return meta
    meta = {"src": tu["src"], "stamp": stamp, "errors": [], "warnings": [], "headers": headers}
    try:
        text = portsrc.fix_cast_lvalues(portsrc.translate_asm(text))
        classes = {}
        if is_cxx:
            text, classes = portsrc.transform_cxx(text, vtables, tukey(tu))
            text = portsrc.mark_deletes(text)
    except Exception as e:  # noqa: BLE001
        meta["errors"].append("transform: %s" % e)
        json.dump(meta, open(meta_path, "w"))
        return meta
    meta["classes"] = classes
    ext = ".cpp" if is_cxx else ".c"
    tsrc = base + ".port" + ext
    open(tsrc, "w", encoding="utf-8").write('#line 1 "%s"\n' % src_path.replace("\\", "/") + text)
    asm = base + ".s"
    cc = tool("g++") if is_cxx else tool("gcc")
    flags = COMMON_FLAGS + (CXX_FLAGS if is_cxx else C_FLAGS)
    rc, so, se = run([cc] + flags + ["-S", tsrc, "-o", asm])
    if rc:
        meta["errors"].append("compile:\n" + se[-4000:])
        json.dump(meta, open(meta_path, "w"))
        return meta
    asm_text = open(asm, encoding="utf-8").read()
    if is_cxx:
        asm_text = split_virtual_defs(asm_text, classes)
    if not X64:
        asm_text = caller_pops_sret(asm_text)
    # --- symbol resolution ----------------------------------------------------------------
    res = Resolver(ctx, tu, is_cxx, classes)
    asm2 = base + ".fix.s"
    open(asm2, "w", encoding="utf-8").write(asm_text)
    rc, so, se = run([tool("as"), "--64" if X64 else "--32", asm2, "-o", base + ".raw.o"])
    if rc:
        meta["errors"].append("assemble:\n" + se[-4000:])
        json.dump(meta, open(meta_path, "w"))
        return meta
    rc, so, se = run([tool("nm"), base + ".raw.o"])
    defined, undefined = [], []
    for line in so.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] == "W" and not WINDOWS:
            parts[1] = "T"  # ELF COMDAT (inline) functions are weak; COFF's are plain globals
        if len(parts) == 2 and parts[0] == "U":
            undefined.append(parts[1])
        elif len(parts) == 3 and parts[1] in "TDBCR":
            defined.append((parts[1], parts[2]))
        elif len(parts) == 3 and parts[1] == "t" and parts[2].startswith("__GLOBAL__sub_I"):
            defined.append(("I", parts[2]))
    renames, info = res.plan(defined, undefined)
    # objcopy can't map two names onto one: later names get an alias of the same address
    taken = set()
    for k in sorted(renames):
        v = renames[k]
        if v in taken:
            n = 1
            while "%s_a%d" % (v, n) in taken:
                n += 1
            v = "%s_a%d" % (v, n)
        taken.add(v)
        renames[k] = v
    meta.update(info)
    meta["psx_syms"] = sorted(v for v in renames.values() if v.startswith("_psx_"))
    meta["aliases"] = sorted(v for v in renames.values()
                             if not v.startswith("_psx_") and re.search(r"_a\d+$", v))
    globalize = info.get("globalize", [])
    if not USCORE:
        # -fleading-underscore named everything `_name`: back to the names the runtime and the
        # generated glue use (`_psx_80012345` -> `psx_80012345`, `_memcpy` -> `memcpy`)
        for line in so.splitlines():
            sym = line.split()[-1] if line.split() else ""
            new = renames.get(sym, sym)
            if new.startswith("__imp__"):
                renames[sym] = "__imp_" + new[7:]  # a DLL import: __imp_ + the C name
            elif new.startswith("_"):
                renames[sym] = new[1:]
        globalize = [strip_us(s) for s in globalize]
    rn = base + ".rename"
    open(rn, "w").write("".join("%s %s\n" % kv for kv in renames.items()))
    # static constructors run from the game's own table (__main), never from the native startup
    cmd = [tool("objcopy"), "--redefine-syms=" + rn, "--remove-section=.ctors",
           "--remove-section=.dtors"]
    if not WINDOWS:
        cmd += ["--remove-section=.init_array", "--remove-section=.fini_array",
                "--remove-section=.rel.init_array", "--remove-section=.rel.fini_array"]
    for s in globalize:
        cmd += ["--globalize-symbol=" + s]
    rc, so, se = run(cmd + [base + ".raw.o", obj])
    if rc:
        meta["errors"].append("objcopy:\n" + se[-2000:])
    json.dump(meta, open(meta_path, "w"), indent=0)
    return meta


SRET_RET_RE = re.compile(r"^(\s*)ret\s+\$4\s*$")
SRET_FIX_RE = re.compile(r"^\s*subl\s+\$4, %esp\s*$")
CALL_RE = re.compile(r"^\s*call\s")
ASM_NOISE_RE = re.compile(r"^\s*(?:[\w.$]+:|\.)")


def caller_pops_sret(asm):
    """egcs/MIPS: the caller owns an aggregate return's hidden pointer, and the decomp declares
    the same function both as `T f(...)` and as `void f(T *, ...)`. SysV i386 has the callee pop
    it (`ret $4`, callers re-adjust with `subl $4, %esp`); undo both so the two forms agree.
    Game code is cdecl-only, so every `ret $4` here is an aggregate return."""
    out = []
    after_call = False
    for line in asm.split("\n"):
        if after_call and SRET_FIX_RE.match(line):
            after_call = False
            continue
        if CALL_RE.match(line):
            after_call = True
        elif not ASM_NOISE_RE.match(line) and line.strip():
            after_call = False
        m = SRET_RET_RE.match(line)
        out.append(m.group(1) + "ret" if m else line)
    return "\n".join(out)


class Resolver:
    def __init__(self, ctx, tu, is_cxx, classes):
        self.ctx, self.tu, self.is_cxx, self.classes = ctx, tu, is_cxx, classes
        self.syms = {k: int(v, 16) for k, v in tu["syms"].items()}
        self.syms_by_sig = {}
        self.syms_by_qual = {}
        for k, a in self.syms.items():
            d = ctx.gnu_dem.get(k, k)
            self.syms_by_sig.setdefault(norm_sig(d), a)
            self.syms_by_qual.setdefault(qual_name(d), set()).add(a)
        self._itan = {}

    def itanium(self, sym):
        if sym not in self._itan:
            self._itan.update(demangle_itanium([sym]))
        return self._itan[sym]

    def data_addr(self, name, cxx_sym=None):
        if name in self.syms:
            return self.syms[name]
        if self.is_cxx and name.startswith("_Z"):
            d = self.itanium(name)
            q = qual_name(d)
            if q in self.syms_by_qual and len(self.syms_by_qual[q]) == 1:
                return next(iter(self.syms_by_qual[q]))
            name = q.replace("::", "_")
        if name in self.ctx.global_syms:
            return self.ctx.global_syms[name]
        return None

    def func_addr_c(self, name):
        if name in self.syms:
            return self.syms[name]
        if name in self.ctx.fn_by_name:
            return self.ctx.fn_by_name[name]
        return self.ctx.global_syms.get(name)  # (a library function another TU names)

    def plan(self, defined, undefined):
        """Return ({old: new}, info) for objcopy."""
        renames, info = {}, {"defines": {}, "unresolved": [], "thunks": [], "dtor_calls": [],
                             "dtor_defs": [], "globalize": [], "dtor_calls_d": [],
                             "del_wrappers": []}
        dem = demangle_itanium([s[1:] for s in undefined + [d for _, d in defined]
                                if s.startswith("__Z")]) if self.is_cxx else {}
        for _, d in defined:  # split_virtual_defs: the definition demangles as its real name
            if d.endswith(VDEF) and d.startswith("__Z"):
                dem[d[1:]] = demangle_itanium([d[1:-len(VDEF)]]).get(d[1:-len(VDEF)], d[1:])
        primary = self.tu["addr"]
        # the TU's own functions (its header comments), by signature, qualified name and name
        own_sig, own_qual, own_name = {}, {}, {}
        for a, n in self.tu["funcs"]:
            d = self.ctx.gnu_dem.get(n, n)
            own_sig.setdefault(norm_sig(d), a)
            own_qual.setdefault(qual_name(d), []).append(a)
            own_name[n] = a

        def own_by_qual(q):
            a = own_qual.get(q, [])
            return a[0] if len(a) == 1 else None
        # ---- definitions
        for kind, sym in defined:
            name = strip_us(sym)
            if kind == "I":
                # a static-object constructor: its retail address is the __main table entry that
                # falls inside the range of one of the TU's functions (the other function in range
                # is the teardown)
                for lo, _ in self.tu["funcs"]:
                    size = self.ctx.functions.get(lo, (0,))[0]
                    a = next((a for a in self.ctx.static_ctors if lo <= a < lo + max(size, 0x400)),
                             None)
                    if a is not None:
                        renames[sym] = "_pcfn_%08x" % a
                        info["globalize"].append("_pcfn_%08x" % a)
                        info["defines"][hex(a)] = name
                        break
                continue
            if kind != "T":
                continue
            addr = None
            if name.startswith("_Z"):
                d = dem.get(name, name)
                sig = norm_sig(d)
                if sig in own_sig:
                    addr = own_sig[sig]
                elif sig in self.ctx.fn_by_sig and len(self.ctx.fn_by_sig[sig]) == 1:
                    addr = self.ctx.fn_by_sig[sig][0]
                elif not re.search(r"C2E|D[12]Ev$", name):
                    # declared with other parameter types than retail's: the TU's one function
                    # of that name
                    addr = own_by_qual(qual_name(d))
                is_dtor = re.search(r"D[12]Ev$", name) is not None
                is_ctor2 = re.search(r"C2E", name) is not None
                if addr is not None and is_ctor2:
                    continue  # C1 carries the address; C2 stays native
                if addr is not None and is_dtor:
                    if name.endswith("D1Ev"):
                        continue
                    cls = qual_name(d).split("::")[0]
                    # D2 (base-object destructor): link.py wraps it at the retail address
                    renames[sym] = "_pcdtor_%08x" % addr
                    info["dtor_defs"].append([addr, cls, self.class_delete_addr(cls)])
                    info["defines"][hex(addr)] = name
                    continue
            else:
                # C: the retail name, or the C name of a C++ one (`Options_Init`, Options_Init__Fv)
                addr = own_name.get(name)
                if addr is None:
                    addr = self.ctx.fn_by_name.get(name)
                if addr is None:
                    addr = own_by_qual(name)
            if addr is not None and addr < CODE_END:
                if re.search(r"C1E", name):
                    # egcs constructors return `this` and C callers use it; g++'s return void.
                    # link.py wraps the native constructor at the retail address.
                    renames[sym] = "_pcctor_%08x" % addr
                    info.setdefault("ctor_defs", []).append(addr)
                else:
                    renames[sym] = "_pcfn_%08x" % addr
                info["defines"][hex(addr)] = name
        # a single-function TU whose function has a placeholder name (`op`) defines its header's
        if len(self.tu["funcs"]) == 1 and hex(primary) not in info["defines"] and \
                not info["dtor_defs"]:
            loose = [sym for kind, sym in defined if kind == "T" and sym not in renames and
                     not re.search(r"C2E|D[12]Ev$", sym)]
            if len(loose) == 1:
                renames[loose[0]] = "_pcfn_%08x" % primary
                info["defines"][hex(primary)] = strip_us(loose[0])
        info["undefined_headers"] = ["%s (0x%08x)" % (n, a) for a, n in self.tu["funcs"]
                                     if hex(a) not in info["defines"]]
        # ---- references
        for sym in undefined:
            name = strip_us(sym)
            if NATIVE_OK.match(sym) or NATIVE_OK.match(name):
                continue
            if name in NATIVE_RENAME:
                renames[sym] = "_" + NATIVE_RENAME[name]
                continue
            if name in self.ctx.data_defined:
                continue  # a global of game/data: linked by its name
            if name.startswith("_Z") and self.is_cxx:
                r = self.resolve_cxx(name, dem.get(name, name), info)
            else:
                a = self.func_addr_c(name)
                r = None if a is None else "_psx_%08x" % a
            if r is not None and r.startswith("_psx_") and int(r[5:13], 16) >= CODE_END:
                r = None  # data: by name (a global another TU defines)
            elif r is None and self.data_addr(name) is None:
                info["unresolved"].append(name)
            if r is None:
                continue
            # (a call by address, FUN_<addr>, to one of NATIVE_RENAME's functions: that one)
            renames[sym] = self.ctx.native_at.get(r, r)
        return renames, info

    def class_delete_addr(self, cls):
        """The operator delete a `delete` of `cls` calls: its own or the nearest base class's."""
        seen = set()
        while cls and cls not in seen:
            seen.add(cls)
            c = self.classes.get(cls)
            if not c:
                break
            if c.get("has_delete"):
                d = "%s::operatordelete(void*)" % cls
                for k, a in self.syms.items():
                    if norm_sig(self.ctx.gnu_dem.get(k, k)) == d:
                        return a
                if d in self.ctx.fn_by_sig:
                    return self.ctx.fn_by_sig[d][0]
            cls = next((b for b in c.get("bases", []) if b in self.classes), None)
        return self.ctx.psyq.get("__builtin_delete")

    def resolve_cxx(self, name, d, info):
        sig = norm_sig(d)
        q = qual_name(d)
        # destructors: D2 = base-class call from a derived dtor, D1 = delete-expression/explicit
        if re.search(r"D[012]Ev$", name):
            cls = q.split("::")[0]
            target = self.syms_by_sig.get(sig)
            if target is None and sig in self.ctx.fn_by_sig:
                target = self.ctx.fn_by_sig[sig][0]
            c = self.classes.get(cls)
            if name.endswith("D1Ev") and c and c["slots"] and "~" in c["slots"]:
                return self.vthunk(cls, "~", info, dtor=True)
            if target is None:
                return None
            if name.endswith("D1Ev"):
                # complete-object destructor: frees when called for a delete statement
                info["dtor_calls_d"].append(target)
                return "_pcdtorcalld_%08x" % target
            info["dtor_calls"].append(target)
            return "_pcdtorcall_%08x" % target
        if sig.endswith("operatordelete(void*)"):
            r = self.resolve_cxx_plain(name, d, sig, q, info)
            if r is None or not r.startswith("_psx_"):
                return r
            a = int(r[5:13], 16)
            info["del_wrappers"].append(a)
            return "_pcdel_%08x" % a
        return self.resolve_cxx_plain(name, d, sig, q, info)

    def resolve_cxx_plain(self, name, d, sig, q, info):
        # `Base::Method(...)`: the method itself, no dispatch (portsrc.mark_nonvirtual_calls)
        nonvirtual = portsrc.NONVIRTUAL + "(" in d
        if nonvirtual:
            d = d.replace(portsrc.NONVIRTUAL + "(", "(")
            sig, q = norm_sig(d), qual_name(d)
        # virtual methods: dispatch through the retail vtable
        if "::" in q and not nonvirtual:
            cls, meth = q.rsplit("::", 1)
            if is_virtual(self.classes, cls, meth, d):
                return self.vthunk(cls, meth, info)
        if sig in self.syms_by_sig:
            return "_psx_%08x" % self.syms_by_sig[sig]
        if sig in BUILTIN_CXX:
            a = self.ctx.psyq.get(BUILTIN_CXX[sig]) or self.syms.get(BUILTIN_CXX[sig])
            if a:
                return "_psx_%08x" % a
        if sig in self.ctx.fn_by_sig and len(self.ctx.fn_by_sig[sig]) == 1:
            return "_psx_%08x" % self.ctx.fn_by_sig[sig][0]
        if q in self.syms_by_qual and len(self.syms_by_qual[q]) == 1:
            return "_psx_%08x" % next(iter(self.syms_by_qual[q]))
        a = self.data_addr(name)
        if a is not None:
            return "_psx_%08x" % a
        return None

    def vthunk(self, cls, meth, info, dtor=False):
        c = self.classes[cls]
        slot = c["slots"].index(meth)
        root = c["root"]
        # the vptr offset is only known after compilation: emitted by the thunk generator from a
        # probe of offsetof(root, __vptr) in this TU (see gen_vptr_probe)
        key = "%s_%s_%d_%s" % (tukey(self.tu), root, slot, "d" if dtor else "m")
        info["thunks"].append({"key": key, "off": "pcvptroff_%s_%s" % (tukey(self.tu), root),
                               "slot": slot, "dtor": dtor})
        return "_pcvt_" + key


# --------------------------------------------------------------------------------------------
# main


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-j", type=int, default=os.cpu_count())
    ap.add_argument("--clean", action="store_true")
    ap.add_argument("--only", default=None)
    ap.add_argument("--no-link", action="store_true")
    ap.add_argument("--release", action="store_true",
                    help="build/pc/release/thps.exe for distribution: no retail data embedded (read "
                         "from the player's disc), no console, debug info in thps.debug")
    ap.add_argument("--version", default=None,
                    help="the build's version, MAJOR[.MINOR[.PATCH[.BUILD]]] with an optional "
                         "-suffix (1.2.0, 1.2.0-beta1): in the exe's version resource (Windows), "
                         "logged at startup, and in the release archive's name. Default: dev")
    a = ap.parse_args()
    if a.version is not None and not re.fullmatch(r"\d+(\.\d+){0,3}(-[0-9A-Za-z.-]+)?",
                                                  a.version):
        ap.error("--version %r: expected MAJOR[.MINOR[.PATCH[.BUILD]]][-suffix]" % a.version)
    toolchain.add_to_path()
    if a.clean and os.path.isdir(os.path.join(OUT, "obj")):
        shutil.rmtree(os.path.join(OUT, "obj"))
    os.makedirs(OUT, exist_ok=True)
    ctx = Ctx()
    tus = [t for t in ctx.tus.values() if not a.only or a.only in t["src"]]
    # the port's own headers too (pc_game.h is force-included; pc_geom.h is inline code): the
    # stamps only see the project headers a TU includes by name
    pc_headers = "".join(open(os.path.join(PC, "include", h), encoding="utf-8").read()
                         for h in sorted(os.listdir(os.path.join(PC, "include")))
                         if h.endswith(".h"))
    flags_hash = hashlib.sha1((json.dumps(COMMON_FLAGS + C_FLAGS + CXX_FLAGS) +
                              json.dumps(sorted(ctx.data_defined)) +
                              open(os.path.join(GAME, "vtables.tsv")).read() +
                              open(os.path.join(HERE, "portsrc.py")).read() +
                              open(__file__).read() + pc_headers).encode()).hexdigest()
    metas = []
    with cf.ThreadPoolExecutor(a.j) as ex:
        futs = {ex.submit(build_tu, ctx, t, flags_hash): t for t in tus}
        for f in cf.as_completed(futs):
            metas.append(f.result())
    errs = [m for m in metas if m["errors"]]
    unresolved = {}
    for m in metas:
        for u in m.get("unresolved", []):
            unresolved.setdefault(u, []).append(m["src"])
    # header comments no definition was matched to (retail functions the port never builds, like
    # a static teardown, or a definition whose name/signature the build can't place)
    no_def = sorted((m["src"], h) for m in metas for h in m.get("undefined_headers", []))
    report = os.path.join(OUT, "build_report.txt")
    with open(report, "w", encoding="utf-8") as f:
        f.write("%d TUs, %d with errors\n\n" % (len(metas), len(errs)))
        for m in sorted(errs, key=lambda m: m["src"]):
            f.write("== %s\n%s\n" % (m["src"], "\n".join(m["errors"])))
        f.write("\n== headers without a definition (%d)\n" % len(no_def))
        for src, h in no_def:
            f.write("%s\t%s\n" % (src, h))
        f.write("\n== unresolved symbols (%d)\n" % len(unresolved))
        for u, srcs in sorted(unresolved.items()):
            f.write("%s\t%s\n" % (u, " ".join(srcs[:4])))
    print("%d TUs, %d errors, %d headers without a definition, %d unresolved symbols -> %s" % (
        len(metas), len(errs), len(no_def), len(unresolved), report))
    json.dump(metas, open(os.path.join(OUT, "tus.json"), "w"))
    if a.no_link or errs:
        return 1 if errs else 0
    import link  # noqa: E402
    return link.link(ctx, metas, OUT, TOOLBIN, release=a.release, version=a.version)


if __name__ == "__main__":
    sys.exit(main())
