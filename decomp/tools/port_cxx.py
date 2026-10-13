#!/usr/bin/env python3
"""Turn a matching C source that spells out a C++ class by hand into a real C++ source for the port.

    python tools/port_cxx.py src/bit/setrgb.c [...]      (run from decomp/)

For each source: `git mv` it to .cpp (a .cpp source stays put) and move its matched.tsv rows to reimpl.tsv (verify `cxx`: the
C++ rewrite of byte-exact C, same logic, not byte-exact; the matching C is in git history). The
rewrite itself is done by hand afterwards. Idempotent: a source already moved is skipped.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def rows(path):
    return open(os.path.join(ROOT, path), encoding="utf-8").read().split("\n")


def main(srcs):
    matched = rows("matched.tsv")
    reimpl = rows("reimpl.tsv")
    while reimpl and not reimpl[-1]:
        reimpl.pop()
    for src in srcs:
        src = src.replace("\\", "/")
        if not src.endswith((".c", ".cpp")):
            sys.exit("%s: not a .c/.cpp source" % src)
        cpp = src if src.endswith(".cpp") else src[:-2] + ".cpp"
        moved = [l for l in matched if l.split("\t")[0] == src]
        if not moved:
            print("%s: no matched.tsv row, skipped" % src)
            continue
        matched = [l for l in matched if l.split("\t")[0] != src]
        for l in moved:
            p = l.split("\t")
            reimpl.append("\t".join([cpp, p[1], p[2], "cxx", p[4]]))
        if src != cpp and os.path.exists(os.path.join(ROOT, src)):
            subprocess.run(["git", "mv", src, cpp], cwd=ROOT, check=True)
        print("%s -> %s (%d row%s)" % (src, cpp, len(moved), "" if len(moved) == 1 else "s"))
    open(os.path.join(ROOT, "matched.tsv"), "w", encoding="utf-8", newline="\n").write("\n".join(matched))
    open(os.path.join(ROOT, "reimpl.tsv"), "w", encoding="utf-8", newline="\n").write("\n".join(reimpl) + "\n")


if __name__ == "__main__":
    main(sys.argv[1:])
