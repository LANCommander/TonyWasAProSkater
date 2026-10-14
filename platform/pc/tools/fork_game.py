#!/usr/bin/env python3
"""One-time fork of the port's game code out of the matching decompilation (2026-10-14).

    python platform/pc/tools/fork_game.py [--force]

Copies what the port builds from decomp/ into game/:
  src/       every TU of decomp/matched.tsv, nearmiss.tsv and reimpl.tsv (and its .syms.json), plus
             the headers next to them
  include/   decomp/include
  data/      decomp/data
  tus.tsv    the TU list (src, addr, size, kind: c | asm, name), from the three lists
  functions.tsv, data_names.tsv
with every PC_PORT (and R3D_HOOKS) conditional resolved to its PC side: the port's sources no
longer have PS1 branches. decomp/ stays as the frozen byte-matching reference.
"""
import glob
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
DECOMP = os.path.join(ROOT, "decomp")
GAME = os.path.join(ROOT, "game")

DEFINED = {"PC_PORT", "R3D_HOOKS"}
COND_RE = re.compile(r"^\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)$")


def resolve(text, path):
    """unifdef for the DEFINED macros: their #ifdef/#ifndef blocks keep the PC side only."""
    out = []
    stack = []  # per open conditional: None (kept as is) or [active] (resolved)
    for line in text.split("\n"):
        m = COND_RE.match(line)
        live = all(s is None or s[0] for s in stack)
        if m:
            kw, rest = m.group(1), m.group(2).split("/*")[0].split("//")[0].strip()
            if kw in ("ifdef", "ifndef"):
                if rest in DEFINED:
                    stack.append([kw == "ifdef"])
                    continue
                stack.append(None)
            elif kw == "if":
                if any(d in rest for d in DEFINED):
                    raise SystemExit("%s: #if on %s: resolve it by hand" % (path, rest))
                stack.append(None)
            elif kw == "elif":
                if stack[-1] is not None:
                    raise SystemExit("%s: #elif in a resolved block" % path)
            elif kw == "else":
                if stack[-1] is not None:
                    stack[-1][0] = not stack[-1][0]
                    continue
            elif kw == "endif":
                if stack.pop() is not None:
                    continue
        if live:
            out.append(line)
    if stack:
        raise SystemExit("%s: unbalanced conditionals" % path)
    s = "\n".join(out)
    return re.sub(r"\n{3,}", "\n\n", s)


def copy(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.splitext(src)[1] in (".c", ".cpp", ".h"):
        text = open(src, encoding="utf-8").read()
        open(dst, "w", encoding="utf-8", newline="\n").write(resolve(text, src))
    else:
        shutil.copyfile(src, dst)


def read_tsv(path):
    for line in open(path, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        yield line.rstrip("\n").split("\t")


def main():
    if os.path.exists(GAME):
        if "--force" not in sys.argv:
            raise SystemExit("game/ exists (--force replaces it)")
        shutil.rmtree(GAME)
    rows, seen = [], set()
    for kind, lst in (("c", "matched.tsv"), ("c", "nearmiss.tsv"), ("asm", "reimpl.tsv")):
        for p in read_tsv(os.path.join(DECOMP, lst)):
            if p[0].endswith(".s") or p[0] in seen:
                continue
            seen.add(p[0])
            name = p[4] if len(p) > 4 else ""
            rows.append((p[0], p[1], p[2], kind, name))
    dirs = set()
    for src, *_ in rows:
        copy(os.path.join(DECOMP, src), os.path.join(GAME, src))
        sj = os.path.splitext(src)[0] + ".syms.json"
        if os.path.exists(os.path.join(DECOMP, sj)):
            copy(os.path.join(DECOMP, sj), os.path.join(GAME, sj))
        dirs.add(os.path.dirname(src))
    for d in dirs:
        for h in glob.glob(os.path.join(DECOMP, d, "*.h")):
            copy(h, os.path.join(GAME, d, os.path.basename(h)))
    for tree in ("include", "data"):
        for f in glob.glob(os.path.join(DECOMP, tree, "**", "*"), recursive=True):
            if os.path.isfile(f):
                copy(f, os.path.join(GAME, os.path.relpath(f, DECOMP)))
    for f in ("functions.tsv", "data_names.tsv"):
        copy(os.path.join(DECOMP, f), os.path.join(GAME, f))
    with open(os.path.join(GAME, "tus.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# src\taddr\tsize\tkind (c: decompiled, asm: hand-written assembly as C)\tname\n")
        for r in rows:
            f.write("\t".join(r) + "\n")
    print("forked %d TUs into %s" % (len(rows), GAME))


if __name__ == "__main__":
    main()
