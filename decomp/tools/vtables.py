#!/usr/bin/env python3
"""Print the retail egcs vtables: every slot's function, by class.

    python tools/vtables.py [CLASS ...]        (run from decomp/; no args: every known vtable)

Vtable addresses come from data_names.tsv and every .syms.json (`__vt_<n><Class>` / `_vt.<n><Class>`).
egcs layout: an 8-byte header, then per slot {short delta, short index, void *pfn}; slot s's
function is at vtable + 12 + 8*s. Slots run until an entry is not a function entry (functions.tsv) or has a nonzero delta.
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = open(os.path.join(ROOT, "..", "extern", "thps1_retail", "SLUS_008.60"), "rb").read()
TEXT_LO, TEXT_HI = 0x80010000, 0x8009a000


def word(a):
    o = a - 0x80010000 + 0x800
    return int.from_bytes(IMG[o:o + 4], "little")


def vtables():
    out = {}
    pat = re.compile(r"^(?:_vt\.|__vt_)(\d+)(\w+)$")
    names = [(p[1], p[0]) for p in (l.split("\t") for l in open(os.path.join(ROOT, "data_names.tsv"),
                                                                    encoding="utf-8")) if len(p) > 1 and p[0].startswith("0x")]
    for f in glob.glob(os.path.join(ROOT, "src", "**", "*.syms.json"), recursive=True):
        names += list(json.load(open(f)).items())
    for k, v in names:
        m = pat.match(k)
        if m and len(m.group(2)) == int(m.group(1)):
            out.setdefault(m.group(2), set()).add(int(v, 16))
    return out


def functions():
    fn = {}
    for l in open(os.path.join(ROOT, "functions.tsv"), encoding="utf-8"):
        p = l.rstrip("\n").split("\t")
        if p[0].startswith("0x"):
            fn[int(p[0], 16)] = p[4] if len(p) > 4 else p[2]
    return fn


def main(want):
    fn = functions()
    for cls, addrs in sorted(vtables().items()):
        if want and cls not in want:
            continue
        for a in sorted(addrs):
            print("%s  vtable 0x%08x" % (cls, a))
            s = 0
            while True:
                pfn = word(a + 12 + 8 * s)
                delta = word(a + 8 + 8 * s) & 0xffff
                # past the end: not a function entry, or a nonzero delta (no multiple inheritance
                # here; what follows a vtable is often a jump table into the middle of a function)
                if not TEXT_LO <= pfn < TEXT_HI or pfn not in fn or delta:
                    break
                print("  slot %d: 0x%08x %s%s" % (s, pfn, fn.get(pfn, "?"),
                                                  "  (delta %d)" % (delta - 0x10000 if delta & 0x8000 else delta) if delta else ""))
                s += 1


if __name__ == "__main__":
    main(set(sys.argv[1:]))
