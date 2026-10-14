"""Decompile the retail executable's initialised data into C: decomp/data/*.c.

    python tools/gen_data.py [--exe ../extern/thps1_retail/SLUS_008.60] [--layouts LAYOUTS.json]

Writes one file per game module (the src/<module> directory of the code that uses the data, from
the TUs' .syms.json), each global defined with its retail address (psxdata.h: PSX_DATA) under the
name the decompiled code uses for it (.syms.json, else data_names.tsv, else D_<addr>). Pointers
are written as references to what they point at (functions, other globals, offsets into them), so
the image the port builds from these files (platform/pc/tools/data_image.py) has the same bytes
as the retail one. Strings are string literals; arrays are typed from the THPS2 demo's symbols
where it has the global, else by their contents. With --layouts (a platform/pc/tools/layouts.py
snapshot of the port's struct layouts), a global the demo types as a struct gets its whole
extent, laid out one element per group of lines; the names the decompiled code gives its fields
and elements become offsets into it.

Left out, and listed in decomp/data/omitted.tsv (data_image.py --check skips them):
  * Psy-Q library data that neither the game code nor the port uses (the port has its own
    library implementations; the few library tables they read are kept);
  * MIPS switch jump tables (compiler output: code addresses only the retail code uses);
  * the memory card icons (art; the port writes saves without the card header).

This was a one-off: it wrote the first version of decomp/data (as .c files, text as globals),
which has been reworked by hand since (.cpp, text as literals, typed tables, decomp/data/fixed;
README.md, Data). Don't re-run it over decomp/data.
"""
import argparse
import collections
import glob
import json
import os
import re
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
DECOMP = os.path.dirname(HERE)
ROOT = os.path.dirname(DECOMP)
OUT = os.path.join(DECOMP, "data")

TEXT = 0x80010000
GAME_END = 0x8008B900  # game code and Neversoft's hand-written assembly; the libraries follow
LIB_END = 0x8009D0E0   # .rodata starts here
DATA_END = 0x800D21C8  # .bss starts here
IMAGE_END = 0x800DD000
GP = 0x800D1080

# data in .text (functions.tsv D_ rows), and the word after .bss that holds the address where free
# memory starts (Init_ResetMemory reads it), are decompiled too
TEXT_DATA = [(0x80071FA4, 0x80071FA4 + 148), (0x800DC94C, 0x800DC950)]

# Psy-Q library data the port's own code reads, kept although only library code references it
PORT_USES = {
    0x800C9670: "rcossin_tbl (pc_libgte.c)",
    0x800CE490: "constructors-done flag (hle_bios.c)",
    0x800CE548: "rsqrt table (pc_libgte.c)",
    0x800CE6D4: "K_COR table (pc_libgte.c)",
    0x800CE704: "ratan_tbl (pc_libgte.c)",
    0x800D0544: "_stacksize (pc_main.c)",
    0x800D0548: "_stacktop (pc_main.c)",
    0x800D0A9C: "sqrt table (pc_libgte.c)",
    0x800D101C: "global constructor table (hle_bios.c)",
}
# names and types for the library data that is kept (the decompiled code has no names for it)
LIB_NAMES = {
    0x800C9670: ("rcossin_tbl", "short[8200]"),  # {sin, cos} for 4096 angles (+4)
    0x800CE490: ("ctors_done", "int"),
    0x800CE548: ("rsqrt_tbl", "ushort[198]"),    # VectorNormal: 4096 / sqrt, n = 64..255
    0x800CE6D4: ("K_cor_tbl", "int[12]"),        # catan's CORDIC angles, atan(2^-i)
    0x800CE704: ("ratan_tbl", "short[1026]"),    # atan(i / 1024), 4096 = 360 degrees
    0x800D0544: ("_stacksize", "int"),
    0x800D0548: ("_stacktop", "int"),
    0x800D0A9C: ("sqrt_tbl", "ushort[192]"),     # SquareRoot0: 4096 * sqrt(n / 64), n = 64..255
    0x800D101C: ("__CTOR_LIST__", ""),           # the global constructors hle_bios.c runs
}
# library data sections (after the game's own .rodata and .data)
LIB_RANGES = [(0x800A3634, 0x800A40B4), (0x800C9490, 0x800D1080)]
ART = {"Tape01IconPalette", "Tape01IconTexture", "Tony01IconPalette", "Tony01IconTexture"}

IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_$]*$")
ADDR_SUFFIX = re.compile(r"_([0-9a-f]{8})$")


def s16(x):
    return x - 0x10000 if x & 0x8000 else x


class Exe:
    def __init__(self, path):
        d = open(path, "rb").read()
        self.base, size = struct.unpack_from("<2I", d, 0x18)
        self.data = d[0x800:0x800 + size]

    def b(self, lo, hi):
        return self.data[lo - self.base:hi - self.base]

    def w(self, a):
        return struct.unpack_from("<I", self.data, a - self.base)[0]


def rows(path):
    for line in open(path, encoding="utf-8"):
        if line.startswith("0x"):
            yield line.rstrip("\n").split("\t")


# ------------------------------------------------------------------------------------- inputs

def load_functions():
    """addr -> (end, name) for every function in the image (game: functions.tsv; libraries:
    the names the decompiled code calls them by, else FUN_<addr>)."""
    funcs = {}
    for p in rows(os.path.join(DECOMP, "functions.tsv")):
        if not p[2].startswith("D_"):
            funcs[int(p[0], 16)] = [int(p[0], 16) + int(p[1]), p[2]]
    return funcs


def load_syms():
    """.syms.json of every TU: [(module, name, addr)]."""
    out = []
    for f in glob.glob(os.path.join(DECOMP, "src", "*", "*.syms.json")):
        module = os.path.basename(os.path.dirname(f))
        for name, a in json.load(open(f)).items():
            if name != "gp":
                out.append((module, name, int(a, 16)))
    return out


def code_refs(exe):
    """addr -> {"game", "lib"}: data addresses formed by lui/addiu pairs and $gp offsets."""
    refs = collections.defaultdict(set)
    hi = {}
    for a in range(TEXT, LIB_END, 4):
        w = exe.w(a)
        op, rs, rt, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
        if op == 0xF:
            hi[rt] = imm << 16
            continue
        if op in (0x8, 0x9, 0xD) or 0x20 <= op <= 0x2E or op in (0x32, 0x3A):
            base = GP if rs == 28 else hi.get(rs)
            if base is not None:
                t = (base + (imm if op == 0xD else s16(imm))) & 0xFFFFFFFF
                if LIB_END <= t < IMAGE_END:
                    refs[t].add("game" if a < GAME_END else "lib")
            if op in (0x8, 0x9, 0xD) or 0x20 <= op <= 0x26:
                hi.pop(rt, None)
            continue
        if op == 0:
            rd, fn = (w >> 11) & 31, w & 0x3F
            # addu rd, base, index: an indexed table (lui base + index, then lw lo(rd))
            if fn == 0x21 and (rs in hi) != (rt in hi):
                hi[rd] = hi[rs] if rs in hi else hi[rt]
            else:
                hi.pop(rd, None)
        if w == 0x03E00008:  # jr ra
            hi.clear()
    return refs


# ------------------------------------------------------------------------------------- naming

def string_at(exe, a, end):
    """The C string at a (printable text, NUL-terminated before end), or None."""
    s = bytearray()
    while a + len(s) < end:
        c = exe.data[a + len(s) - exe.base]
        if c == 0:
            break
        if not (32 <= c < 127 or c in (9, 10, 13) or c >= 0xA0):
            return None
        s.append(c)
    else:
        return None
    if not s or sum(32 <= c < 127 for c in s) < len(s) * 0.8:
        return None
    return bytes(s)


def string_name(s, taken):
    words = re.findall(rb"[A-Za-z0-9]+", s)
    base = "s_" + "_".join(w.decode() for w in words)[:32] if words else "s_"
    return base


class Namer:
    def __init__(self):
        self.used = {}

    def take(self, name, a):
        if not name or not IDENT.match(name):
            name = "D_%08x" % a
        if name in self.used and self.used[name] != a:
            name = "%s_%08x" % (name, a)
        self.used[name] = a
        return name


# ------------------------------------------------------------------------------------- objects

class Obj:
    def __init__(self, a, name, demo_type=""):
        self.a, self.name, self.demo_type = a, name, demo_type
        self.b = None
        self.kind = None  # "string", "omit", or None (array)
        self.reason = ""
        self.module = None


def build_objects(exe, funcs, syms, data_names, refs, dptrs):
    namer = Namer()
    by_addr = collections.defaultdict(collections.Counter)
    for module, name, a in syms:
        if LIB_END <= a < DATA_END or any(lo <= a < hi for lo, hi in TEXT_DATA):
            by_addr[a][name] += 2
    for p in data_names:
        a = int(p[0], 16)
        by_addr[a][p[1]] += 1
    demo = {int(p[0], 16): p[5] if len(p) > 5 else "" for p in data_names}

    starts = {}
    ranges = [(LIB_END, DATA_END)] + TEXT_DATA
    for a, names in sorted(by_addr.items()):
        if not any(lo <= a < hi for lo, hi in ranges):
            continue
        best = sorted(names.items(), key=lambda kv: (-kv[1], kv[0]))[0][0]
        starts[a] = Obj(a, namer.take(best, a), demo.get(a, ""))
    for a, (name, t) in LIB_NAMES.items():
        starts[a] = Obj(a, namer.take(name, a), t)
    for lo, hi in TEXT_DATA:
        starts.setdefault(lo, Obj(lo, namer.take("D_%08x" % lo, lo)))
    if LIB_END not in starts:
        starts[LIB_END] = Obj(LIB_END, None)
    kinds = {int(p[0], 16): p[2] for p in data_names}
    typed_extents(starts, kinds)

    # A named global ends where its size says (a string, a demo-typed array), else at the next
    # name or the next address a pointer in the data points at. What follows a sized global up to
    # the next name is a gap: split at those addresses, then into strings, jump tables and the
    # rest. (Game code references don't split: the decompiled code names every global it uses, and
    # the other addresses it forms are fields and elements. Library code's do, in its sections.)
    extra = sorted(a for a in dptrs | {r for r, who in refs.items() if "lib" in who and any(
        lo <= r < hi for lo, hi in LIB_RANGES)} if LIB_END <= a < DATA_END and a not in starts
        and a % 4 == 0)
    objs = []
    for lo, hi in ranges:
        named = [a for a in sorted(starts) if lo <= a < hi]
        if not named or named[0] != lo:
            named = [lo] + named
            starts.setdefault(lo, Obj(lo, None))
        for i, a in enumerate(named):
            nxt = named[i + 1] if i + 1 < len(named) else hi
            o = starts[a]
            end = known_size(exe, o, nxt)
            cuts = [x for x in extra if a < x < nxt and (end is None or x >= end)]
            if end is not None and end < nxt:
                cuts = sorted(set(cuts) | {end})
            pts = [a] + cuts + [nxt]
            o.b = pts[1]
            objs.append(o)
            for p, q in zip(pts[1:], pts[2:]):
                g = Obj(p, None)
                g.b = q
                objs.append(g)

    # inside unnamed objects, carve out strings and jump tables
    out = []
    for o in objs:
        if o.name is not None:
            out.append(o)
            continue
        out.extend(carve(exe, funcs, o.a, o.b, namer))
    out = split_jump_tables(exe, funcs, out, namer)
    # named objects that are strings
    for o in out:
        if o.kind is None and (o.demo_type.startswith("char") or o.name.startswith("s_")):
            s = string_at(exe, o.a, o.b)
            if s is not None and not any(exe.b(o.a + len(s), o.b)):
                o.kind = "string"
    return out, namer


TYPE_SIZE = {"char": 1, "uchar": 1, "short": 2, "ushort": 2, "int": 4, "uint": 4, "long": 4,
             "ulong": 4}


# sizes of the types the demo gives globals that the port's headers don't define as classes
EXTRA_SIZE = {"CVECTOR": 4, "SVECTOR": 8, "VECTOR": 16, "MATRIX": 32, "RECT": 8, "DR_TPAGE": 8,
              "CdlLOC": 4, "SMovieDetails": 32}
STRUCT_SIZE = {}  # from a layouts.py snapshot (--layouts)


def elem_size(t):
    """Size of one element of a demo type ("SStats[14]" -> 340), or None."""
    base = t.split("[")[0]
    if base.endswith("*"):
        return 4
    if base in TYPE_SIZE:
        return TYPE_SIZE[base]
    if base in STRUCT_SIZE:
        return STRUCT_SIZE[base]
    if base in EXTRA_SIZE:
        return EXTRA_SIZE[base]
    if re.match(r"E[A-Z]", base):  # enums
        return 4
    return None


def typed_extents(starts, kinds):
    """Give each named global of a struct type (or an array of one) its extent: whole elements up
    to the next global that is one in its own right. The names inside it (a field the decompiled
    code addresses directly, a demo offset name) are dropped: pointers there become offsets into
    it. Returns the addresses dropped."""
    def soft(a, o):
        k = kinds.get(a)
        if k in ("offset", "layout"):
            return True
        return k is None and re.match(r"(DAT|D)_[0-9a-f]{8}$", o.name or "") is not None

    hard = [a for a, o in sorted(starts.items()) if not soft(a, o)]
    dropped = set()
    for i, a in enumerate(hard):
        o = starts[a]
        t = o.demo_type
        base = t.split("[")[0]
        if not t or base in TYPE_SIZE or base.endswith("*") or a in dropped:
            continue
        size = elem_size(t)
        if not size:
            continue
        nxt = hard[i + 1] if i + 1 < len(hard) else DATA_END
        count = (nxt - a) // size if "[" in t else min(1, (nxt - a) // size)
        if count < 1:
            continue
        o.elem, o.count, o.fixed_end = size, count, min(nxt, (a + size * count + 3) & ~3)
        for b in list(starts):
            if a < b < o.fixed_end:
                dropped.add(b)
                del starts[b]
    return dropped


def known_size(exe, o, nxt):
    """End of a named global whose size is known (a string; a demo-typed array of a basic or
    pointer type; a struct, typed_extents), aligned up to 4; None if unknown."""
    if o.name is None:
        return None
    if getattr(o, "fixed_end", None):
        return o.fixed_end
    t = o.demo_type
    if t in ("char[]", "") and o.name.startswith("s_") or t == "char[]":
        s = string_at(exe, o.a, nxt)
        if s is not None:
            return min(nxt, (o.a + len(s) + 1 + 3) & ~3)
        return None
    m = re.match(r"(\w+)(\*?)((?:\[\d+\])+)$", t)
    if not m:
        return None
    esz = 4 if m.group(2) else TYPE_SIZE.get(m.group(1))
    if esz is None:
        return None
    n = 1
    for d in re.findall(r"\[(\d+)\]", m.group(3)):
        n *= int(d)
    end = (o.a + esz * n + 3) & ~3
    return end if end <= nxt else None


def carve(exe, funcs, lo, hi, namer):
    out = []
    a = lo
    cur = None

    def flush(end):
        nonlocal cur
        if cur is not None:
            cur.b = end
            out.append(cur)
            cur = None

    while a < hi:
        # skip zero padding between pieces
        if exe.data[a - exe.base] == 0 and cur is None:
            a += 1
            continue
        s = string_at(exe, a, hi) if a % 4 == 0 else None
        if s is not None and len(s) >= 2:
            flush(a)
            o = Obj(a, namer.take(string_name(s, namer.used), a), "char[]")
            o.kind = "string"
            end = a + len(s) + 1
            while end < hi and end % 4 and exe.data[end - exe.base] == 0:
                end += 1
            o.b = end
            out.append(o)
            a = end
            continue
        jt = jump_table(exe, funcs, a, hi) if a % 4 == 0 else 0
        if jt:
            flush(a)
            o = Obj(a, namer.take("jtbl_%08x" % a, a))
            o.kind, o.reason, o.b = "omit", "jump table", a + jt
            out.append(o)
            a += jt
            continue
        if cur is None:
            start = a & ~3 if (a & ~3) >= lo else a
            cur = Obj(start, namer.take("D_%08x" % start, start))
        a += 4 if a % 4 == 0 else 1
    flush(hi)
    return [o for o in out if o.b > o.a]


def split_jump_tables(exe, funcs, objs, namer):
    """Cut the jump tables out of the objects that run into them (a vtable followed by a switch's
    table): the object ends where the table starts, the table is left out, and what follows is
    carved like any unnamed data."""
    out = []
    for o in objs:
        if o.kind is not None or o.a % 4:
            out.append(o)
            continue
        pieces = []  # (start, end, is a jump table)
        a = start = o.a
        while a + 4 <= o.b:
            # an unnamed global that is nothing but code addresses is a piece of a jump table the
            # matching build's anchors cut up, however short
            whole = a == o.a and re.match(r"(DAT|D)_[0-9a-f]{8}$", o.name or "") and \
                jump_table(exe, funcs, a, o.b, 1) == o.b - o.a
            n = o.b - o.a if whole else jump_table(exe, funcs, a, o.b)
            if n:
                if a > start:
                    pieces.append((start, a, False))
                pieces.append((a, a + n, True))
                a = start = a + n
            else:
                a += 4
        if start < o.b:
            pieces.append((start, o.b, False))
        if len(pieces) == 1 and not pieces[0][2]:
            out.append(o)
            continue
        for s, e, jt in pieces:
            if jt:
                x = Obj(s, namer.take("jtbl_%08x" % s, s))
                x.kind, x.reason, x.b = "omit", "jump table", e
                out.append(x)
            elif s == o.a:
                o.b = e
                out.append(o)
            else:
                out.extend(carve(exe, funcs, s, e, namer))
        # a struct array cut short keeps whole elements only
        if getattr(o, "elem", None):
            if (o.b - o.a) % o.elem:
                o.elem = None
            else:
                o.count = (o.b - o.a) // o.elem
    return out


def containing_function(funcs, t, cache={}):
    if not cache:
        cache["k"] = sorted(funcs)
    import bisect
    keys = cache["k"]
    i = bisect.bisect_right(keys, t) - 1
    if i >= 0 and t < funcs[keys[i]][0]:
        return keys[i]
    return None


def jump_table(exe, funcs, a, hi, least=2):
    """Bytes of the jump table at a: >= `least` words pointing at instructions inside (not at the
    start of) one function."""
    n, f0 = 0, None
    while a + 4 * n + 4 <= hi:
        t = exe.w(a + 4 * n)
        if not (TEXT <= t < GAME_END) or t in funcs or t % 4:
            break
        f = containing_function(funcs, t)
        if f is None or (f0 is not None and f != f0):
            break
        f0 = f
        n += 1
    return 4 * n if n >= least else 0


# ------------------------------------------------------------------------------------- output

class Emitter:
    def __init__(self, exe, funcs, objs, data_names, syms):
        self.exe, self.funcs, self.objs = exe, funcs, objs
        self.obj_keys = [o.a for o in objs]
        # names for addresses outside the generated data (.bss): exact names only
        self.bss = {}
        for module, name, a in syms:
            if DATA_END <= a < IMAGE_END and IDENT.match(name):
                self.bss.setdefault(a, collections.Counter())[name] += 1
        for p in data_names:
            a = int(p[0], 16)
            if DATA_END <= a < IMAGE_END and IDENT.match(p[1]):
                self.bss.setdefault(a, collections.Counter())[p[1]] += 1
        self.bss = {a: c.most_common(1)[0][0] for a, c in self.bss.items()}
        self.lib_names = {}
        for module, name, a in syms:
            if GAME_END <= a < LIB_END and IDENT.match(name):
                self.lib_names.setdefault(a, name)
        # what data_image.py resolves names with: name -> {addr}
        self.names = collections.defaultdict(set)
        for a, (end, name) in funcs.items():
            self.names[name].add(a)
        for module, name, a in syms:
            self.names[name].add(a)
        for p in data_names:
            self.names[p[1]].add(int(p[0], 16))
        self.objnames = {o.name: o.a for o in objs if o.kind != "omit"}

    def obj_at(self, t):
        import bisect
        i = bisect.bisect_right(self.obj_keys, t) - 1
        if i >= 0 and t < self.objs[i].b:
            return self.objs[i]
        return None

    def symbol(self, t):
        """(name, offset) for a pointer value, or None if it's a plain number. A function or .bss
        name that data_image.py can't resolve to this one address by name (two functions called
        _._5CItem, a file static) gets the address as a suffix."""
        r = self._symbol(t)
        if r is None:
            return None
        name, off = r
        a = t - off
        if self.objnames.get(name) == a:
            return r
        if (self.names.get(name) != {a} or name in self.objnames) and \
                not name.endswith("_%08x" % a):
            name = "%s_%08x" % (name, a)
        return name, off

    def _symbol(self, t):
        if TEXT <= t < LIB_END:
            if t in self.funcs:
                return self.funcs[t][1], 0
            if t in self.lib_names:
                return self.lib_names[t], 0
            # inside a function: only jump tables (left out) point there; elsewhere it's a number
            # (flag words like 0x80010001)
            if containing_function(self.funcs, t) is not None:
                return None
            o = self.obj_at(t)
            if o is not None and o.kind != "omit":
                return o.name, t - o.a
            return "FUN_%08x" % t, 0
        if LIB_END <= t < DATA_END:
            o = self.obj_at(t)
            if o is None or o.kind == "omit":
                return "D_%08x" % t, 0
            return o.name, t - o.a
        if DATA_END <= t < IMAGE_END:
            return self.bss.get(t, "D_%08x" % t), 0
        return None


def c_string(s):
    out = []
    for c in s:
        if c == 10:
            out.append("\\n")
        elif c == 9:
            out.append("\\t")
        elif c == 13:
            out.append("\\r")
        elif c == 34:
            out.append('\\"')
        elif c == 92:
            out.append("\\\\")
        elif c == 63 and out and out[-1] == "?":  # no trigraphs
            out.append("\\?")
        elif 32 <= c < 127:
            out.append(chr(c))
        else:
            out.append("\\%03o" % c)
    return "".join(out)


def string_lines(s, indent):
    """A literal, split after a line's \\n (and before 90 columns) into adjacent literals."""
    parts, cur = [], b""
    for i, c in enumerate(s):
        cur += bytes([c])
        if c == 10 and i + 1 < len(s) and s[i + 1] != 10 and cur.strip(b"\n") or \
                len(c_string(cur)) > 80:
            parts.append(cur)
            cur = b""
    if cur or not parts:
        parts.append(cur)
    lits = ['"%s"' % c_string(p) for p in parts]
    return lits[0] if len(lits) == 1 else ("\n" + indent).join(lits)


ELEM = {1: "unsigned char", 2: "short", 4: "int"}
DEMO_ELEM = {"char": 1, "uchar": 1, "short": 2, "ushort": 2}
UNSIGNED = {"uchar": "unsigned char", "ushort": "unsigned short", "uint": "unsigned int",
            "ulong": "unsigned int"}


def c_ident(name):
    """A C identifier for a symbol name (egcs destructors _._9CSmokeJet -> dtor_9CSmokeJet,
    _GLOBAL_.I.x -> _GLOBAL__I_x); the declaration carries the real name (psxdata.h PSX_NAME)."""
    if IDENT.match(name):
        return name
    if name.startswith("_._"):
        name = "dtor" + name[2:]
    return re.sub(r"[^A-Za-z0-9_$]", "_", name)


def emit_object(em, o, externs):
    exe = em.exe
    data = exe.b(o.a, o.b)
    # what the THPS2 demo's symbols say the global is, when they have it
    comment = ""
    if getattr(o, "elem", None):
        base = o.demo_type.split("[")[0]
        comment = "/* %s%s (THPS2 demo: %s) */\n" % (
            base, "[%d]" % o.count if "[" in o.demo_type else "", o.demo_type)
    elif o.demo_type and o.demo_type not in ("char[]",):
        comment = "/* THPS2 demo: %s */\n" % o.demo_type
    head = "PSX_DATA(0x%08x) " % o.a
    if o.kind == "string":
        s = data.split(b"\0", 1)[0]
        size = len(data)
        dims = "[%d]" % size if size != len(s) + 1 and o.a % 4 == 0 and size % 4 == 0 and \
            not (size - len(s) - 1 < 4) else "[]"
        lit = string_lines(s, "\t")
        o.ctype = "char"
        return "%s%schar %s%s = %s;\n" % (comment, head, o.name, dims, lit)

    base = o.demo_type.split("[")[0].rstrip("*").strip()
    ptr = o.demo_type.endswith("*") or "*" in o.demo_type
    size = o.b - o.a
    if base in DEMO_ELEM and not ptr:
        esz = DEMO_ELEM[base]
    else:
        esz = 4
    while esz > 1 and (o.a % esz or size % esz):
        esz //= 2
    etype = ELEM[esz]
    if not ptr and base in UNSIGNED and DEMO_ELEM.get(base, 4) == esz:
        etype = UNSIGNED[base]
    o.ctype = etype
    n = size // esz
    vals = [int.from_bytes(data[i * esz:(i + 1) * esz], "little") for i in range(n)]

    def render(v):
        if esz == 4:
            sym = em.symbol(v)
            if sym is not None:
                name, off = sym
                externs.add(name)
                name = c_ident(name)
                return "(int)%s" % name if off == 0 else "(int)(%s + 0x%x)" % (name, off)
            sv = v - (1 << 32) if v & 0x80000000 else v
            return str(sv) if -4096 <= sv <= 4096 else "0x%x" % v
        if esz == 2:
            sv = v - 0x10000 if v & 0x8000 and etype == "short" else v
            return str(sv) if -4096 <= sv <= 4096 else "0x%x" % v
        return "0x%02x" % v

    while vals and vals[-1] == 0:
        vals.pop()
    if not vals:
        return "%s%s%s %s[%d];\n" % (comment, head, etype, o.name, n)
    # an array of structs: one group of lines per element
    per = o.elem // esz if getattr(o, "elem", None) and o.count > 1 and o.elem % esz == 0 else 0
    items, i = [], 0
    while i < len(vals):
        if per and i % per == 0:
            items.append(("elem", i // per))
        if vals[i] == 0:
            j = i
            while j < len(vals) and vals[j] == 0:
                j += 1
            if j - i >= 16:
                items.append(("skip", j))
                i = j
                continue
        items.append(("v", render(vals[i])))
        i += 1
    lines, cur = [], "\t"
    for kind, x in items:
        if kind == "elem":
            if cur.endswith("= "):
                cur += "/* [%d] */ " % x
                continue
            if cur.strip():
                lines.append(cur.rstrip())
            cur = "\t/* [%d] */ " % x
            continue
        if kind == "skip":
            if cur.strip():
                lines.append(cur.rstrip())
            cur = "\t[%d] = " % x
            continue
        piece = x + ","
        if len(cur.expandtabs(4)) + len(piece) + 1 > 100 and cur.strip() and not cur.endswith("= "):
            lines.append(cur.rstrip())
            cur = "\t"
        cur += piece if cur.endswith("\t") or cur.endswith("= ") or cur.endswith("*/ ") \
            else " " + piece
    if cur.strip():
        lines.append(cur.rstrip())
    body = "\n".join(lines)
    return "%s%s%s %s[%d] = {\n%s\n};\n" % (comment, head, etype, o.name, n, body)


def assign_modules(objs, syms):
    import bisect
    keys = [o.a for o in objs]
    votes = collections.defaultdict(collections.Counter)
    for module, name, a in syms:
        i = bisect.bisect_right(keys, a) - 1
        if i >= 0 and a < objs[i].b:
            votes[i][module] += 1
    prev = "misc"
    for i, o in enumerate(objs):
        if o.a in LIB_NAMES or any(lo <= o.a < hi for lo, hi in LIB_RANGES) and not votes[i]:
            o.module = "psyq"
        elif votes[i]:
            o.module = sorted(votes[i].items(), key=lambda kv: (-kv[1], kv[0]))[0][0]
        else:
            o.module = prev
        prev = o.module if o.module != "psyq" else prev


def classify_omitted(objs, refs):
    for o in objs:
        if o.kind == "omit":
            continue
        if o.name in ART:
            o.kind, o.reason = "omit", "memory card icon (art; the port writes no card header)"
            continue
        in_lib = any(lo <= o.a < hi for lo, hi in LIB_RANGES)
        if not in_lib:
            continue
        if any(o.a <= a < o.b for a in PORT_USES):
            continue
        users = set()
        for a in range(o.a, o.b):
            users |= refs.get(a, set())
        if "game" not in users:
            o.kind, o.reason = "omit", "Psy-Q library data (the port has its own libraries)"


HEADER = """/* Decompiled data: the retail executable's initialised globals that %s uses, each at
 * its PS1 address (psxdata.h). Generated by tools/gen_data.py; edit by hand from now on. Pointers
 * are references to what they point at; platform/pc/tools/data_image.py builds the port's data
 * image from these files and checks it against the retail bytes. */
#include "psxdata.h"
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(ROOT, "extern", "thps1_retail", "SLUS_008.60"))
    ap.add_argument("--layouts", help="a platform/pc/tools/layouts.py snapshot: struct sizes, so "
                                      "struct-typed globals get their whole extent")
    a = ap.parse_args()
    if a.layouts:
        sizes = collections.defaultdict(collections.Counter)
        for classes in json.load(open(a.layouts)).values():
            for name, c in classes.items():
                sizes[name][c["size"]] += 1
        STRUCT_SIZE.update({n: c.most_common(1)[0][0] for n, c in sizes.items()})
    exe = Exe(a.exe)
    funcs = load_functions()
    syms = load_syms()
    data_names = list(rows(os.path.join(DECOMP, "data_names.tsv")))
    refs = code_refs(exe)
    # every global the decompiled game code names is used by the game, however it addresses it
    for module, name, a in syms:
        if LIB_END <= a < IMAGE_END:
            refs[a].add("game")
    # data -> data pointers count as uses by whatever uses the pointing object; a pointer from
    # game data into library data keeps it
    dptrs = set()
    for p in range(LIB_END, DATA_END, 4):
        t = exe.w(p)
        if LIB_END <= t < DATA_END and not any(lo <= p < hi for lo, hi in LIB_RANGES):
            refs[t].add("game")
            dptrs.add(t)
    objs, namer = build_objects(exe, funcs, syms, data_names, refs, dptrs)
    classify_omitted(objs, refs)
    assign_modules(objs, syms)
    em = Emitter(exe, funcs, objs, data_names, syms)

    os.makedirs(OUT, exist_ok=True)
    for f in glob.glob(os.path.join(OUT, "*.c")):
        os.remove(f)
    files = collections.defaultdict(list)
    for o in objs:
        if o.kind != "omit":
            files[o.module].append(o)
    defined = {o.name for o in objs if o.kind != "omit"}
    total = 0
    for module, os_ in sorted(files.items()):
        externs = set()
        bodies = [emit_object(em, o, externs) for o in os_]
        own = {o.name: o for o in os_}
        ext = sorted(externs - set(own))
        # globals of this file that pointers refer to (declared first, so order doesn't matter)
        fwd = sorted(externs & set(own))
        who = "the Psy-Q libraries' code (kept: the game or the port reads them)" \
            if module == "psyq" else "src/%s/" % module
        text = HEADER % who
        if ext:
            text += "\n" + "".join(
                "extern char %s[];\n" % e if c_ident(e) == e else
                "extern char %s[] PSX_NAME(\"%s\");\n" % (c_ident(e), e) for e in ext)
        if fwd:
            text += "\n" + "".join("extern %s %s[];\n" % (own[n].ctype, n) for n in fwd)
        # one-line globals in a row stay together; anything longer gets blank lines around it
        out = ""
        for k, body in enumerate(bodies):
            short = body.count("\n") == 1
            prev_short = k > 0 and bodies[k - 1].count("\n") == 1
            out += ("" if k == 0 or short and prev_short else "\n") + body
        text += "\n" + out
        open(os.path.join(OUT, module + ".c"), "w", encoding="utf-8", newline="\n").write(text)
        total += len(os_)
    with open(os.path.join(OUT, "omitted.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# Retail data the port's data image leaves out (zero), checked by data_image.py "
                "--check. Generated by tools/gen_data.py.\n# start\tend\tname\treason\n")
        for o in objs:
            if o.kind == "omit":
                f.write("0x%08x\t0x%08x\t%s\t%s\n" % (o.a, o.b, o.name, o.reason))
    om = [o for o in objs if o.kind == "omit"]
    print("%d objects in %d files; %d left out (%d bytes)" % (
        total, len(files), len(om), sum(o.b - o.a for o in om)))


if __name__ == "__main__":
    main()
