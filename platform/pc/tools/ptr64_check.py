#!/usr/bin/env python3
"""Find the game code's views of memory that disagree in a 64-bit build.

    python platform/pc/tools/ptr64_check.py [--only SUBSTR] [--json OUT]

Needs both builds: build/pc (32-bit) and build/pc64 (THPS_ARCH=x64). Reads the debug information
of every game TU's object in both, and of the data (game/data), and collects the *views* of each
piece of shared memory the code has:

  * a global at a retail address (the data file's definition, every TU's declaration of it),
  * a function's parameters and return value (its definition, every TU's prototype of it), and
    through pointers, the structs they point at.

Two views of the same memory must agree, in the 64-bit build, on where every field the views
share lands (a TU's local struct that skips a pointer with `char _0[0x10]` puts what follows at
its retail offset, the real class 8 bytes later: "offset"), and on what is a pointer (an `int`
holding a pointer loses its top half: "type"). The views are compared field by field (scalar
leaves, by retail offset) against the view that knows most (the data file's, else the one with
most pointers). Prints each disagreement: the TU, the entity, the retail offset, both views.
"""
import argparse
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
sys.path.insert(0, HERE)
from layouts import intval, parse_dwarf, ref  # noqa: E402
import toolchain  # noqa: E402

OUT32 = os.path.join(ROOT, "build", "pc" if toolchain.WINDOWS else "pc-linux")
OUT64 = os.path.join(ROOT, "build", "pc64" if toolchain.WINDOWS else "pc64-linux")
GAME = os.path.join(ROOT, "game")

STRIP = ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type", "DW_TAG_restrict_type")
AGG = ("DW_TAG_structure_type", "DW_TAG_class_type", "DW_TAG_union_type")
PTRS = ("DW_TAG_pointer_type", "DW_TAG_reference_type")
MAX_DEPTH = 2  # pointers followed from a view


class Dwarf:
    """One object's debug information."""

    def __init__(self, path, bits):
        dump = subprocess.run([toolchain.tool("objdump", bits), "--dwarf=info", path],
                              capture_output=True, text=True, errors="replace",
                              env=toolchain.env(bits)).stdout
        self.d = parse_dwarf(dump)
        self.ptr = bits // 8

    def strip(self, die):
        n = 0
        while die is not None and die["tag"] in STRIP and n < 50:
            die = self.d.get(ref(die["attrs"].get("DW_AT_type")))
            n += 1
        return die

    def typ(self, die):
        return self.d.get(ref(die["attrs"].get("DW_AT_type"))) if die else None

    def size(self, die):
        die = self.strip(die)
        if die is None:
            return 0
        s = intval(die["attrs"].get("DW_AT_byte_size"))
        if s is not None:
            return s
        if die["tag"] == "DW_TAG_array_type":
            return self.size(self.typ(die)) * self.count(die)
        if die["tag"] in PTRS:
            return self.ptr
        return 0

    def count(self, arr):
        n = 1
        for c in arr["children"]:
            if c["tag"] == "DW_TAG_subrange_type":
                ub = intval(c["attrs"].get("DW_AT_upper_bound"))
                cnt = intval(c["attrs"].get("DW_AT_count"))
                n *= cnt if cnt is not None else (ub + 1 if ub is not None else 0)
        return n

    def kind(self, die):
        die = self.strip(die)
        if die is None:
            return "void"
        t = die["tag"]
        if t in PTRS or t == "DW_TAG_ptr_to_member_type":
            return "ptr"
        if t == "DW_TAG_enumeration_type":
            return "int%d" % self.size(die)
        if t == "DW_TAG_base_type":
            enc = die["attrs"].get("DW_AT_encoding", "")
            return ("float%d" if "float" in enc else "int%d") % self.size(die)
        return t[7:]

    def tname(self, die):
        die = self.strip(die)
        if die is None:
            return "void"
        if die["tag"] in PTRS:
            return self.tname(self.typ(die)) + "*"
        if die["tag"] == "DW_TAG_array_type":
            return "%s[%d]" % (self.tname(self.typ(die)), self.count(die))
        return die["attrs"].get("DW_AT_name", "<anon %s>" % die["tag"][7:])

    def members(self, die):
        out = []
        for c in die["children"]:
            if c["tag"] == "DW_TAG_inheritance" or (
                    c["tag"] == "DW_TAG_member" and "DW_AT_bit_size" not in c["attrs"] and
                    "DW_AT_data_bit_offset" not in c["attrs"] and
                    ("DW_AT_data_member_location" in c["attrs"] or
                     die["tag"] == "DW_TAG_union_type")):
                out.append(c)
        return out


def loc(m):
    return intval(m["attrs"].get("DW_AT_data_member_location")) or 0


# ------------------------------------------------------------------------------ compact layouts
#
# A layout: {"name": type name, "size": retail size, "f": [field]}, field = one of
#   ["s", o32, o64, kind, size32, name]                 a scalar (kind: int4, ptr, float8, ...)
#   ["b", o32, o64, len32, name]                        opaque bytes (char arrays, pads)
#   ["a", o32, o64, n, es32, es64, layout, name]        an array of non-byte elements
#   ["p", o32, o64, layout, name]                       a pointer to a struct (its layout)
# Pointers are followed MAX_DEPTH levels.


def compact(a, ta, b, tb, depth=0, cache=None):
    cache = {} if cache is None else cache
    ta, tb = a.strip(ta), b.strip(tb)
    key = (ta["off"] if ta else None, tb["off"] if tb else None, depth)
    if key in cache:
        return cache[key]
    lay = {"name": a.tname(ta), "size": a.size(ta), "f": []}
    cache[key] = lay
    if ta is not None and tb is not None:
        fields(a, ta, b, tb, 0, 0, "", lay["f"], depth, cache)
    return lay


def fields(a, ta, b, tb, o32, o64, name, out, depth, cache):
    ta, tb = a.strip(ta), b.strip(tb)
    if ta is None or tb is None:
        return
    tag = ta["tag"]
    if tag in AGG:
        for ma, mb in zip(a.members(ta), b.members(tb)):
            fields(a, a.typ(ma), b, b.typ(mb), o32 + loc(ma), o64 + loc(mb),
                   name + "." + ma["attrs"].get("DW_AT_name", "<base>"), out, depth, cache)
        return
    if tag == "DW_TAG_array_type":
        ea, eb = a.typ(ta), b.typ(tb)
        es32, es64, n = a.size(ea), b.size(eb), a.count(ta)
        # (`extern T x[]`: unknown bound; its first element at least, and for structs the second,
        # so a view whose stride is only right in 32-bit shows; `extern int x[]` views of one
        # object stay single)
        n = max(n, 2 if es32 >= 8 else 1)
        if es32 <= 1:
            out.append(["b", o32, o64, n * max(es32, 1), name])
        elif n:
            out.append(["a", o32, o64, n, es32, es64, compact(a, ea, b, eb, depth, cache), name])
        return
    k = a.kind(ta)
    if k == "ptr" and depth < MAX_DEPTH:
        pa, pb = a.strip(a.typ(ta)), b.strip(b.typ(tb))
        if pa is not None and pb is not None and pa["tag"] in AGG and \
                "DW_AT_declaration" not in pa["attrs"] and "DW_AT_declaration" not in pb["attrs"]:
            out.append(["p", o32, o64, compact(a, pa, b, pb, depth + 1, cache), name])
            return
    out.append(["s", o32, o64, k, a.size(ta), name])


def place(lay, off):
    """(64-bit offset, kind, retail size, field name, pointee layout) of what is at retail
    offset `off` of a layout: a scalar's start (kind), a byte of opaque bytes ("bytes", maps
    linearly), inside a scalar (kind + "+"), or None."""
    for f in lay["f"]:
        t, o32, o64 = f[0], f[1], f[2]
        if off < o32:
            continue
        if t == "s":
            if off == o32:
                return (o64, f[3], f[4], f[5], None)
            if off < o32 + f[4] and f[3] != "ptr":
                return (o64 + off - o32, f[3] + "+", 0, f[5], None)
        elif t == "p":
            if off == o32:
                return (o64, "ptr", 4, f[4], f[3])
        elif t == "b":
            if off < o32 + f[3]:
                return (o64 + off - o32, "bytes", 1, f[4], None)
        elif t == "a":
            n, es32, es64, el = f[3], f[4], f[5], f[6]
            if off < o32 + n * es32:
                i, r = divmod(off - o32, es32)
                p = place(el, r)
                if p is None:
                    return None
                return (o64 + i * es64 + p[0], p[1], p[2], "%s[%d]%s" % (f[7], i, p[3]), p[4])
    return None


def starts(lay, base=0, out=None, depth=0):
    """[(retail offset, kind)] of a layout's scalar leaves (big arrays: elements 0, 1, n-1)."""
    out = [] if out is None else out
    if depth > 30:
        return out
    for f in lay["f"]:
        t, o32 = f[0], f[1]
        if t in ("s", "p"):
            out.append((base + o32, "ptr" if t == "p" else f[3]))
        elif t == "a":
            n, es32, el = f[3], f[4], f[6]
            for i in sorted({0, 1, n - 1} if n > 3 else set(range(n))):
                starts(el, base + o32 + i * es32, out, depth + 1)
    return out


def compare(v, w, out, what, depth=0, seen=None):
    """Layout v's disagreements with layout w -> out [(kind, what, off, v's, w's)]."""
    seen = set() if seen is None else seen
    key = (id(v), id(w))
    if key in seen or depth > MAX_DEPTH:
        return
    seen.add(key)
    for o, _ in starts(v):
        pv, pw = place(v, o), place(w, o)
        if pv is None or pw is None:
            continue
        kv, kw = pv[1], pw[1]
        if "bytes" in (kv, kw):
            continue
        if pv[0] != pw[0]:
            out.append(("offset", what, o, "%s %s at +0x%x" % (pv[3] or "(itself)", kv, pv[0]),
                        "%s %s at +0x%x" % (pw[3] or "(itself)", kw, pw[0])))
        elif (kv == "ptr") != (kw == "ptr") and not kv.endswith("+") and not kw.endswith("+") \
                and 4 in (pv[2], pw[2]):
            out.append(("type", what, o, "%s %s" % (pv[3] or "(itself)", kv),
                        "%s %s" % (pw[3] or "(itself)", kw)))
        if pv[4] is not None and pw[4] is not None:
            compare(pv[4], pw[4], out, "%s->%s" % (what, (pv[3] or "").lstrip(".") or "*"),
                    depth + 1, seen)


# ---------------------------------------------------------------------------------- entities


def cu_children(dw):
    for die in dw.d.values():
        if die["tag"] == "DW_TAG_compile_unit":
            return die["children"]
    return []


def entity_name(dw, die):
    at = die["attrs"]
    spec = dw.d.get(ref(at.get("DW_AT_specification"))) or dw.d.get(
        ref(at.get("DW_AT_abstract_origin")))
    sat = spec["attrs"] if spec else {}
    link = at.get("DW_AT_linkage_name") or sat.get("DW_AT_linkage_name")
    return link or at.get("DW_AT_name") or sat.get("DW_AT_name")


def entities(dw):
    """{name: DIE} of the CU's variables and functions (definitions preferred; class members
    through their specification)."""
    out = {}

    def add(die):
        if die["tag"] not in ("DW_TAG_variable", "DW_TAG_subprogram"):
            return
        n = entity_name(dw, die)
        if not n:
            return
        n = n.split()[-1]
        prev = out.get(n)
        if prev is None or ("DW_AT_declaration" in prev["attrs"] and
                            "DW_AT_declaration" not in die["attrs"]):
            out[n] = die

    for die in cu_children(dw):
        add(die)
        if die["tag"] in AGG or die["tag"] == "DW_TAG_namespace":
            for c in die["children"]:
                add(c)
    return out


def params(dw, die):
    ps = [c for c in die["children"] if c["tag"] == "DW_TAG_formal_parameter"]
    if not ps:
        spec = dw.d.get(ref(die["attrs"].get("DW_AT_specification")))
        if spec:
            ps = [c for c in spec["children"] if c["tag"] == "DW_TAG_formal_parameter"]
    return ps


def rename_map(base):
    """{asm symbol: retail address} from the TU's .rename (objcopy's renames)."""
    out = {}
    p = base + ".rename"
    if os.path.exists(p):
        for line in open(p):
            old, new = line.split()
            m = re.search(r"(?:psx|pcfn|pcctor|pcdtor|pcdtorcall|pcdtorcalld|pcdel)_([0-9a-f]{8})",
                          new)
            if m:
                out[old] = int(m.group(1), 16)
    return out


def scalar_view(a, ta, b, tb):
    """A view of a parameter/return/variable: a struct's layout, or a one-field layout for a
    scalar or pointer (what the pointer points at, through "p")."""
    sa = a.strip(ta)
    if sa is not None and sa["tag"] in AGG:
        return compact(a, ta, b, tb)
    lay = {"name": a.tname(ta), "size": a.size(ta), "f": []}
    if sa is not None:
        fields(a, ta, b, tb, 0, 0, "", lay["f"], 0, {})
    return lay


def tu_views(src):
    """(src, [(key, what, layout)], error): key = ('var', addr) / ('param', addr, i) /
    ('ret', addr), or ('name', name) for a variable linked by its C name."""
    try:
        rel = os.path.splitext(src)[0]
        b32 = os.path.join(OUT32, "obj", rel)
        b64 = os.path.join(OUT64, "obj", rel)
        if not (os.path.exists(b32 + ".o") and os.path.exists(b64 + ".o")):
            return src, [], "missing objects"
        a, b = Dwarf(b32 + ".o", 32), Dwarf(b64 + ".o", 64)
        ren = rename_map(b32)
        us = "_" if toolchain.WINDOWS else ""
        ea, eb = entities(a), entities(b)
        out = []
        for n, da in ea.items():
            addr = ren.get(us + n, ren.get(n))
            if n not in eb:
                continue
            db = eb[n]
            # a definition (not just a declaration) is the reference view of its entity
            defn = "DW_AT_declaration" not in da["attrs"]
            if addr is None:
                # linked by its C name (a data global's label, or another TU's native global)
                if da["tag"] == "DW_TAG_variable":
                    ta, tb = a.typ(da), b.typ(db)
                    if ta is not None and tb is not None:
                        out.append((("name", n), n, scalar_view(a, ta, b, tb), defn))
                continue
            if da["tag"] == "DW_TAG_variable":
                ta, tb = a.typ(da), b.typ(db)
                if ta is not None and tb is not None:
                    out.append((("var", addr), n, scalar_view(a, ta, b, tb), defn))
                continue
            out.append((("ret", addr), n + " return", scalar_view(a, a.typ(da), b, b.typ(db)),
                        defn))
            for i, (pa, pb) in enumerate(zip(params(a, da), params(b, db))):
                out.append((("param", addr, i), "%s arg %d" % (n, i),
                            scalar_view(a, a.typ(pa), b, b.typ(pb)), defn))
        return src, out, None
    except Exception as e:  # noqa: BLE001
        return src, [], "error: %r" % e


def data_views():
    """The data files' globals: [(('var', addr), name, layout, file)]."""
    import data_image  # noqa: E402
    out = []
    d32 = os.path.join(OUT64, "dataobj32")
    d64 = os.path.join(OUT64, "dataobj")
    for src, fixed in data_image.data_sources():
        o = os.path.basename(src)[:-4] + ".o"
        p32, p64 = os.path.join(d32, o), os.path.join(d64, o)
        if not (os.path.exists(p32) and os.path.exists(p64)):
            continue
        a, b = Dwarf(p32, 32), Dwarf(p64, 64)
        ea, eb = entities(a), entities(b)
        obj = data_image.read_object(p32)
        for k, s in enumerate(obj.sections):
            m = data_image.NATIVE_SEC.match(s["name"])
            if not m:
                continue
            for sym in obj.symbols:
                if sym and sym["sec"] == k + 1 and sym["cls"] == 2:
                    n = data_image.c_name(sym["name"], 32)
                    if n in ea and n in eb:
                        out.append((("var", int(m.group(1), 16)), n,
                                    scalar_view(a, a.typ(ea[n]), b, b.typ(eb[n])),
                                    "data/" + os.path.basename(src)))
    return out


def richness(v):
    """The reference view: the data file's, else a definition's, else the one with most
    pointers and fields."""
    tu, what, lay, defn = v
    st = starts(lay)
    return (tu.startswith("data/"), defn, sum(1 for _, k in st if k == "ptr"), len(st))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", help="report only TUs whose path contains this")
    ap.add_argument("--json", help="write the findings here")
    ap.add_argument("-j", type=int, default=os.cpu_count())
    a = ap.parse_args()
    tus = []
    for line in open(os.path.join(GAME, "tus.tsv"), encoding="utf-8"):
        if not line.startswith("#") and line.strip():
            tus.append(line.split("\t")[0])
    tus = list(dict.fromkeys(tus))
    groups, errs = {}, []
    with cf.ProcessPoolExecutor(a.j) as ex:
        for src, views, err in ex.map(tu_views, tus, chunksize=4):
            if err:
                errs.append((src, err))
            for key, what, lay, defn in views:
                groups.setdefault(tuple(key), []).append((src, what, lay, defn))
    for key, what, lay, f in data_views():
        groups.setdefault(key, []).append((f, what, lay, True))
        if ("name", what) in groups:  # (TUs that use the data global by its C name)
            groups[("name", what)].append((f, what, lay, True))
    findings = []
    for key, views in groups.items():
        if len(views) < 2:
            continue
        ref_v = max(views, key=richness)
        for v in views:
            if v is ref_v:
                continue
            out = []
            compare(v[2], ref_v[2], out, v[1])
            # (and what only the reference has fields for)
            back = []
            compare(ref_v[2], v[2], back, v[1])
            seen = {(k, w, o) for k, w, o, _, _ in out}
            out += [(k, w, o, m, t) for k, w, o, t, m in back if (k, w, o) not in seen]
            for kind, what, off, mine, theirs in dict.fromkeys(out):
                findings.append({"tu": v[0], "kind": kind, "what": what, "off": off,
                                 "key": list(key), "mine": mine, "ref": theirs,
                                 "ref_tu": ref_v[0]})
        # a view reaching past the data global's end means into the globals after it, which an
        # x64 build doesn't keep next to it (zero ones go to .bss; layouts grow)
        data_v = next((x for x in views if x[0].startswith("data/") and x[3]), None)
        if data_v and data_v[2]["size"]:
            end = data_v[2]["size"]
            for v in views:
                if v is data_v:
                    continue
                f0 = v[2]["f"]
                if len(f0) == 1 and f0[0][0] == "a" and f0[0][1] == 0 and f0[0][4] == end:
                    continue  # (`extern T x[]` of one T: its second element is ours)
                past =sorted({o for o, _ in starts(v[2]) if o >= end})
                if past:
                    findings.append({"tu": v[0], "kind": "span", "what": v[1], "off": past[0],
                                     "key": list(key), "mine": "%d fields from +0x%x" % (
                                         len(past), past[0]),
                                     "ref": "size 0x%x" % end, "ref_tu": data_v[0]})
    findings.sort(key=lambda f: (f["tu"], f["what"], f["off"]))
    for src, e in errs:
        print("SKIP %s: %s" % (src, e))
    for f in findings:
        if not a.only or a.only in f["tu"]:
            print("%s: %s %s +0x%x: %s; %s has %s" % (f["tu"], f["kind"], f["what"], f["off"],
                                                      f["mine"], f["ref_tu"], f["ref"]))
    print("%d findings in %d TUs (%d offset, %d type, %d span)" % (
        len(findings), len({f["tu"] for f in findings}),
        sum(f["kind"] == "offset" for f in findings), sum(f["kind"] == "type" for f in findings),
        sum(f["kind"] == "span" for f in findings)))
    if a.json:
        json.dump(findings, open(a.json, "w"), indent=1)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
