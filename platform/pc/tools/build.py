#!/usr/bin/env python3
"""Build the native Windows port of THPS1 (see platform/pc/README.md).

    python platform/pc/tools/build.py [-j N] [--clean] [--only SUBSTR] [--no-link] [--mips]

Pipeline for every decompiled TU (decomp/matched.tsv, plus the near-miss sources for functions
that aren't byte-exact yet, and decomp/reimpl.tsv: hand-written assembly reimplemented in C):

  1. source transforms (portsrc.py): inline GTE asm -> software GTE calls; for C++, the egcs
     virtual-function ABI on top of non-virtual native classes;
  2. compile to assembly (i686 GCC, cdecl everywhere: -mabi=sysv);
  3. definitions of globals that live in the PS1 image lose their labels, so every access becomes
     an external reference;
  4. assemble, then rename symbols: every reference resolves to `psx_<addr>` (an absolute symbol at
     the retail address, from the TU's .syms.json), and the TU's own function becomes
     `pcfn_<addr>`.

Then it generates the glue (linker script of absolute symbols, the function table the runtime
turns into trampolines, virtual-dispatch thunks, stubs for anything not implemented yet) and links
everything with the runtime into build/pc/thps.exe.
"""
import argparse
import concurrent.futures as cf
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
DECOMP = os.path.join(ROOT, "decomp")
OUT = os.path.join(ROOT, "build", "pc")
TOOLBIN = os.environ.get("THPS_MINGW", r"C:\msys64\mingw32\bin")
sys.path.insert(0, HERE)
import portsrc  # noqa: E402

GAME_CODE_END = 0x800858AC
CODE_END = 0x8009D0E0  # hand-written assembly rewritten in C (reimpl.tsv) lies past the game code

COMMON_FLAGS = [
    "-m32", "-mabi=sysv", "-mno-ms-bitfields", "-malign-double", "-O1", "-g",
    "-fno-strict-aliasing", "-fwrapv", "-fno-delete-null-pointer-checks", "-fno-builtin",
    "-fno-omit-frame-pointer", "-w", "-DPC_PORT=1",
    # outgoing args are stored, never pushed, so the only post-call %esp adjustment is the
    # aggregate-return one caller_pops_sret removes
    "-maccumulate-outgoing-args",
    # plain char is unsigned in the PS1 ABI (egcs on MIPS): the decompiled code is verified with it
    "-funsigned-char",
    "-I", os.path.join(PC, "include", "psyq"), "-I", os.path.join(PC, "include"),
    "-I", os.path.join(DECOMP, "include"),
    "-include", os.path.join(PC, "include", "pc_game.h"),
]
C_FLAGS = ["-std=gnu89", "-fpermissive", "-fcommon"]
# -fno-lifetime-dse: egcs destructors re-store their class's vptr (portsrc adds the store); g++ would
# drop stores into an object whose lifetime is ending, and those vptrs are read by the base dtors
CXX_FLAGS = ["-std=gnu++98", "-fpermissive", "-fno-exceptions", "-fno-rtti",
             "-fno-threadsafe-statics", "-fno-use-cxa-atexit", "-fno-lifetime-dse"]


def tool(name):
    return os.path.join(TOOLBIN, name + ".exe")


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
    for p in read_tsv(os.path.join(DECOMP, "functions.tsv")):
        if not p[0].startswith("0x"):
            continue
        fn[int(p[0], 16)] = (int(p[1]), p[2], p[4] if len(p) > 4 else p[2])
    return fn


HEADER_RE = re.compile(r"^/\*\s*(\S+)\s+\((0x[0-9a-fA-F]+)\)(.*?)\*/")


def load_tus():
    tus = {}
    for kind in ("matched.tsv", "nearmiss.tsv", "reimpl.tsv"):
        for p in read_tsv(os.path.join(DECOMP, kind)):
            src = p[0]
            if src.endswith(".s") or src in tus:
                continue
            tus[src] = {"src": src, "addr": int(p[1], 16), "exact": kind == "matched.tsv"}
    for src, tu in tus.items():
        path = os.path.join(DECOMP, src)
        first = open(path, encoding="utf-8", errors="replace").readline()
        m = HEADER_RE.match(first)
        tu["name"] = m.group(1) if m else None
        sj = os.path.splitext(path)[0] + ".syms.json"
        tu["syms"] = json.load(open(sj)) if os.path.exists(sj) else {}
    return tus


def load_data_names():
    names = {}
    for p in read_tsv(os.path.join(DECOMP, "data_names.tsv")):
        if p[0].startswith("0x"):
            names.setdefault(p[1], int(p[0], 16))
    return names


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
    return d


def qual_name(d):
    i = d.find("(")
    return re.sub(r"\s+", "", d if i < 0 else d[:i])


# --------------------------------------------------------------------------------------------
# per-TU build


class Ctx:
    """Read-only data every worker needs."""

    def __init__(self):
        self.functions = load_functions()
        self.fn_by_name = {}
        for a, (sz, mangled, dem) in self.functions.items():
            self.fn_by_name.setdefault(mangled, a)
        # hand-written assembly rewritten in C: callable by its label from other rewrites
        for p in read_tsv(os.path.join(DECOMP, "reimpl.tsv")):
            if len(p) > 4 and int(p[1], 16) >= GAME_CODE_END:
                self.fn_by_name.setdefault(p[4], int(p[1], 16))
        self.fn_by_sig = {}
        self.fn_by_qual = {}
        for a, (sz, mangled, dem) in self.functions.items():
            self.fn_by_sig.setdefault(norm_sig(dem), []).append(a)
            self.fn_by_qual.setdefault(qual_name(dem), []).append(a)
        self.data_names = load_data_names()
        self.tus = load_tus()
        # global name -> address, from every TU's syms.json (names with one address only)
        seen = {}
        for tu in self.tus.values():
            for k, v in tu["syms"].items():
                if k.startswith(".") or k == "gp":
                    continue
                seen.setdefault(k, set()).add(int(v, 16))
        self.global_syms = {k: next(iter(v)) for k, v in seen.items() if len(v) == 1}
        # every class's retail vtable known anywhere (a class used by several TUs has one vtable;
        # names with two addresses, i.e. two classes of one name, are left to each TU's syms.json)
        vt = {k: "0x%08x" % v for k, v in self.data_names.items()}
        vt.update({k: "0x%08x" % v for k, v in self.global_syms.items()})
        self.vtables = vtables_of(vt)
        allnames = set()
        for tu in self.tus.values():
            allnames.update(k for k in tu["syms"] if not k.startswith("."))
        allnames.update(m for (_, m, _) in self.functions.values())
        self.gnu_dem = demangle_gnu(sorted(allnames))
        img = open(os.path.join(ROOT, "extern", "thps1_retail", "SLUS_008.60"), "rb").read()
        self.image = img
        off = 0x800D101C - 0x80010000 + 0x800
        self.static_ctors = [int.from_bytes(img[off + 4 * i:off + 4 * i + 4], "little")
                             for i in range(19)]
        self.psyq = {}
        p = os.path.join(ROOT, "build", "psyq_names.tsv")
        if os.path.exists(p):
            for q in read_tsv(p):
                self.psyq.setdefault(q[1], int(q[0], 16))


BUILTIN_CXX = {
    "operatornew(unsignedint)": "__builtin_new",
    "operatordelete(void*)": "__builtin_delete",
    "operatornew[](unsignedint)": "__builtin_vec_new",
    "operatordelete[](void*)": "__builtin_vec_delete",
}

# symbols the native toolchain provides; never remapped
NATIVE_OK = re.compile(r"^(__alloca|___chkstk.*|__[a-z]*div[sd]i3|__[a-z]*mod[sd]i3|__mul[sd]i3|"
                       r"__ashldi3|__ashrdi3|__lshrdi3|__cmpdi2|__ucmpdi2|_?memcpy|_?memset|"
                       r"_?memmove|__fixunsdfdi|__floatdidf|_?abort|_?PC_Gte\w+|__ZTV.*|___cxa_.*|"
                       r"___gxx_personality.*|__Unwind.*|___dso_handle|_atexit|"
                       r"__GLOBAL_OFFSET_TABLE_|_?pc_delete_mark|_?pc_vdelete)$")


def strip_us(sym):
    return sym[1:] if sym.startswith("_") else sym


def vtables_of(syms):
    """{class: addr} from a TU's syms.json vtable keys (_vt.5CItem / __vt_5CItem)."""
    out = {}
    for k, v in syms.items():
        m = re.match(r"^(?:_vt\.|__vt_)(\d+)(\w+)$", k)
        if m and len(m.group(2)) == int(m.group(1)):
            out[m.group(2)] = int(v, 16)
    return out


INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]+)"[^\n]*$', re.M)
HEADER_DIR = os.path.join(DECOMP, "include")


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
    src_path = os.path.join(DECOMP, tu["src"])
    base = tu_paths(tu)
    os.makedirs(os.path.dirname(base), exist_ok=True)
    is_cxx = tu["src"].endswith(".cpp")
    obj = base + ".o"
    meta_path = base + ".json"
    text, headers = expand_includes(open(src_path, encoding="utf-8", errors="replace").read(),
                                    src_path)
    stamp = hashlib.sha1((text +
                          json.dumps(tu["syms"], sort_keys=True) + flags_hash).encode()).hexdigest()
    if os.path.exists(obj) and os.path.exists(meta_path):
        meta = json.load(open(meta_path))
        if meta.get("stamp") == stamp:
            return meta
    meta = {"src": tu["src"], "stamp": stamp, "errors": [], "warnings": [], "headers": headers}
    try:
        text = portsrc.fix_cast_lvalues(portsrc.translate_asm(text))
        classes = {}
        if is_cxx:
            vtables = dict(ctx.vtables)
            vtables.update(vtables_of(tu["syms"]))
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
    asm_text = caller_pops_sret(asm_text)
    # --- symbol resolution ----------------------------------------------------------------
    res = Resolver(ctx, tu, is_cxx, classes)
    asm_text, relocated = relocate_data_defs(asm_text, res)
    meta["relocated"] = relocated
    asm2 = base + ".fix.s"
    open(asm2, "w", encoding="utf-8").write(asm_text)
    rc, so, se = run([tool("as"), "--32", asm2, "-o", base + ".raw.o"])
    if rc:
        meta["errors"].append("assemble:\n" + se[-4000:])
        json.dump(meta, open(meta_path, "w"))
        return meta
    rc, so, se = run([tool("nm"), base + ".raw.o"])
    defined, undefined = [], []
    for line in so.splitlines():
        parts = line.split()
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
    rn = base + ".rename"
    open(rn, "w").write("".join("%s %s\n" % kv for kv in renames.items()))
    cmd = [tool("objcopy"), "--redefine-syms=" + rn, "--remove-section=.ctors",
           "--remove-section=.dtors"]
    for s in info.get("globalize", []):
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


DATA_SECT_RE = re.compile(r"^\s*\.(data|bss|section\s+\.(?:rdata|data|bss)\S*)", re.M)


def relocate_data_defs(asm, res):
    """Drop the definition of every global data symbol that lives in the PS1 image, so references to
    it become external (resolved to the retail address at link time)."""
    relocated = []
    globls = set(re.findall(r"^\s*\.globl\s+(\S+)", asm, re.M))
    out_lines = []
    lines = asm.split("\n")
    section = ".text"
    for line in lines:
        s = line.strip()
        m = re.match(r"\.(text|data|bss)\b", s)
        if m:
            section = "." + m.group(1)
        m = re.match(r"\.section\s+([^,\s]+)", s)
        if m:
            section = m.group(1)
        m = re.match(r"\.comm\s+([^,\s]+)\s*,", s)
        if m:
            sym = m.group(1)
            addr = res.data_addr(strip_us(sym))
            if addr is not None:
                relocated.append((sym, addr))
                continue
        # file statics (`static int x;`): the matching build placed them at retail addresses too,
        # listed in the TU's own .syms.json (never resolved through the global maps)
        m = re.match(r"\.lcomm\s+([^,\s]+)\s*,", s)
        if m:
            sym = m.group(1)
            addr = res.static_addr(sym)
            if addr is not None:
                relocated.append((sym, addr))
                continue
        m = re.match(r"^([^\s:]+):$", s)
        if m and not section.startswith(".text"):
            sym = m.group(1)
            if sym in globls:
                addr = res.data_addr(strip_us(sym), cxx_sym=sym)
            else:
                addr = res.static_addr(sym)
            if addr is not None:
                relocated.append((sym, addr))
                out_lines.append(line.replace(sym + ":", sym + "$pcdead:"))
                continue
        out_lines.append(line)
    rel = {s for s, _ in relocated}
    out = "\n".join(l for l in out_lines if not re.match(r"^\s*\.globl\s+(\S+)\s*$", l) or
                    re.match(r"^\s*\.globl\s+(\S+)", l).group(1) not in rel)
    res.relocated = dict(relocated)
    return out, relocated


class Resolver:
    def __init__(self, ctx, tu, is_cxx, classes):
        self.ctx, self.tu, self.is_cxx, self.classes = ctx, tu, is_cxx, classes
        self.syms = {k: int(v, 16) for k, v in tu["syms"].items()}
        self.relocated = {}
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
        if name in self.ctx.data_names:
            return self.ctx.data_names[name]
        return None

    def static_addr(self, sym):
        """Retail address of a file-static data symbol, from this TU's .syms.json only. C statics
        are `_name`, C++ namespace-scope statics `__ZL<len><name>`."""
        name = strip_us(sym)
        m = re.match(r"_ZL(\d+)(\w+)$", name)
        if m and len(m.group(2)) == int(m.group(1)):
            name = m.group(2)
        return self.syms.get(name)

    def func_addr_c(self, name):
        if name in self.syms:
            return self.syms[name]
        if name in self.ctx.fn_by_name:
            return self.ctx.fn_by_name[name]
        return None

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
        primary_name = self.tu.get("name")
        # ---- definitions
        for kind, sym in defined:
            name = strip_us(sym)
            if kind == "I":
                # a static-object constructor: its retail address is the __main table entry that
                # falls inside this TU's range (the other function in range is the teardown)
                lo, size = primary, self.ctx.functions.get(primary, (0,))[0]
                for a in self.ctx.static_ctors:
                    if lo <= a < lo + max(size, 0x400):
                        renames[sym] = "_pcfn_%08x" % a
                        info["globalize"].append("_pcfn_%08x" % a)
                        info["defines"][hex(a)] = name
                continue
            if kind != "T":
                continue
            addr = None
            if name.startswith("_Z"):
                d = dem.get(name, name)
                sig = norm_sig(d)
                pd = self.ctx.gnu_dem.get(primary_name, primary_name) if primary_name else None
                if pd and norm_sig(pd) == sig:
                    addr = primary
                elif sig in self.ctx.fn_by_sig and len(self.ctx.fn_by_sig[sig]) == 1:
                    addr = self.ctx.fn_by_sig[sig][0]
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
                if primary_name and name == primary_name:
                    addr = primary
                else:
                    addr = self.ctx.fn_by_name.get(name)
            if addr is not None and addr < CODE_END:
                if re.search(r"C1E", name):
                    # egcs constructors return `this` and C callers use it; g++'s return void.
                    # link.py wraps the native constructor at the retail address.
                    renames[sym] = "_pcctor_%08x" % addr
                    info.setdefault("ctor_defs", []).append(addr)
                else:
                    renames[sym] = "_pcfn_%08x" % addr
                info["defines"][hex(addr)] = name
        # the earliest sources carry no header and a placeholder name (`op`): a TU that defines a
        # single unmapped function defines the manifest's function
        if hex(primary) not in info["defines"] and not info["dtor_defs"]:
            loose = [sym for kind, sym in defined if kind == "T" and sym not in renames and
                     not re.search(r"C2E|D[12]Ev$", sym)]
            if len(loose) == 1:
                renames[loose[0]] = "_pcfn_%08x" % primary
                info["defines"][hex(primary)] = strip_us(loose[0])
        # ---- references
        for sym in undefined:
            name = strip_us(sym)
            if NATIVE_OK.match(sym) or NATIVE_OK.match(name):
                continue
            if sym in self.relocated:
                renames[sym] = "_psx_%08x" % self.relocated[sym]
                continue
            if name.startswith("_Z") and self.is_cxx:
                r = self.resolve_cxx(name, dem.get(name, name), info)
            else:
                a = self.func_addr_c(name)
                if a is None:
                    a = self.data_addr(name)
                r = None if a is None else "_psx_%08x" % a
            if r is None:
                info["unresolved"].append(name)
            else:
                renames[sym] = r
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
        if sig in self.syms_by_sig:
            return "_psx_%08x" % self.syms_by_sig[sig]
        # virtual methods: dispatch through the retail vtable
        if "::" in q:
            cls, meth = q.rsplit("::", 1)
            c = self.classes.get(cls)
            if c and meth in c["slots"]:
                return self.vthunk(cls, meth, info)
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
    ap.add_argument("--mips", action="store_true",
                    help="also translate the retail MIPS (THPS_MIPS / THPS_SNAP debugging)")
    a = ap.parse_args()
    os.environ["PATH"] = TOOLBIN + os.pathsep + os.environ["PATH"]
    if a.clean and os.path.isdir(os.path.join(OUT, "obj")):
        shutil.rmtree(os.path.join(OUT, "obj"))
    os.makedirs(OUT, exist_ok=True)
    ctx = Ctx()
    tus = [t for t in ctx.tus.values() if not a.only or a.only in t["src"]]
    flags_hash = hashlib.sha1((json.dumps(COMMON_FLAGS + C_FLAGS + CXX_FLAGS) +
                              open(os.path.join(HERE, "portsrc.py")).read() +
                              open(__file__).read()).encode()).hexdigest()
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
    report = os.path.join(OUT, "build_report.txt")
    with open(report, "w", encoding="utf-8") as f:
        f.write("%d TUs, %d with errors\n\n" % (len(metas), len(errs)))
        for m in sorted(errs, key=lambda m: m["src"]):
            f.write("== %s\n%s\n" % (m["src"], "\n".join(m["errors"])))
        f.write("\n== unresolved symbols (%d)\n" % len(unresolved))
        for u, srcs in sorted(unresolved.items()):
            f.write("%s\t%s\n" % (u, " ".join(srcs[:4])))
    print("%d TUs, %d errors, %d unresolved symbols -> %s" % (len(metas), len(errs),
                                                             len(unresolved), report))
    json.dump(metas, open(os.path.join(OUT, "tus.json"), "w"))
    if a.no_link or errs:
        return 1 if errs else 0
    import link  # noqa: E402
    return link.link(ctx, metas, OUT, TOOLBIN, mips=a.mips)


if __name__ == "__main__":
    sys.exit(main())
