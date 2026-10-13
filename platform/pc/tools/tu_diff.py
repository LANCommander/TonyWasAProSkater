#!/usr/bin/env python3
"""Compare TUs' build results (build/pc/obj/<tu>.json) with a saved build/pc/tus.json.

    python platform/pc/tools/tu_diff.py BASE_TUS.json [SUBSTR ...]

For refactors that must not change what a TU links to: per TU, the functions it defines, every
retail address it references (psx_syms), constructor/destructor wiring, destructor and operator
delete calls, and virtual-dispatch thunks (class root + slot). Prints differences; exit 1 if any.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OBJ = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(HERE))), "build", "pc", "obj")


def summary(m):
    # key: <tu>_<root class>_<slot>_<m|d>
    thunks = ["%s slot %d%s" % (t["key"].rsplit("_", 3)[1], t["slot"], " dtor" if t["dtor"] else "")
              for t in m.get("thunks", [])]
    return {
        "defines": sorted(m.get("defines", {})),
        "psx_syms": sorted(re.sub(r"_a\d+$", "", s) for s in set(m.get("psx_syms", []))),
        "ctor_defs": sorted(m.get("ctor_defs", [])),
        "dtor_defs": sorted(map(tuple, m.get("dtor_defs", []))),
        "dtor_calls": sorted(set(m.get("dtor_calls", []))),
        "dtor_calls_d": sorted(set(m.get("dtor_calls_d", []))),
        "del_wrappers": sorted(set(m.get("del_wrappers", []))),
        "thunks": sorted(set(thunks)),
        "errors": m.get("errors", []),
        "unresolved": sorted(m.get("unresolved", [])),
    }


def main():
    base = {m["src"]: m for m in json.load(open(sys.argv[1]))}
    subs = sys.argv[2:]
    bad = n = 0
    for src, b in sorted(base.items()):
        if subs and not any(s in src for s in subs):
            continue
        p = os.path.join(OBJ, os.path.splitext(src)[0] + ".json")
        if not os.path.exists(p):
            continue
        cur = json.load(open(p))
        n += 1
        sb, sc = summary(b), summary(cur)
        diffs = []
        for k in sb:
            if sb[k] != sc[k]:
                gone = [x for x in sb[k] if x not in sc[k]]
                new = [x for x in sc[k] if x not in sb[k]]
                diffs.append("  %s: -%s +%s" % (k, gone, new))
        if diffs:
            bad += 1
            print(src + "\n" + "\n".join(diffs))
    print("%d TUs compared, %d differ" % (n, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
