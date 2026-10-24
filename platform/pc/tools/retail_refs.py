#!/usr/bin/env python3
"""Write game/retail_refs.tsv: the addresses the build takes from the retail executable, so the
build doesn't need SLUS_008.60 (they are addresses only, no retail code or data).

    python platform/pc/tools/retail_refs.py [--exe extern/thps1_retail/SLUS_008.60]

  static_ctor  the __main table's 19 static-object constructors, in table order (build.py: a TU's
               static constructor is the entry inside one of its functions)
  lib_call     every Psy-Q library address the retail code calls (jal, bltzal, bgezal), whether
               the port implements it or not (link.py: names and stubs)

Run it again only if the address ranges below change.
"""
import argparse
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(ROOT, "game", "retail_refs.tsv")

TEXT_START = 0x80010000
GAME_END = 0x800858AC  # first byte after the game code (decomp/functions.tsv)
LIB_END = 0x8009D0E0   # first byte of .rodata after the Psy-Q libraries
CTOR_TABLE = 0x800D101C
CTORS = 19


def jal_targets(img):
    """Every target of a jal / bltzal / bgezal in the code (img: the text, from TEXT_START)."""
    out = set()
    for off in range(0, LIB_END - TEXT_START, 4):
        w = int.from_bytes(img[off:off + 4], "little")
        if (w >> 26) == 3:
            out.add(0x80000000 | ((w & 0x3FFFFFF) << 2))
        elif (w >> 26) == 1 and ((w >> 16) & 31) in (16, 17):  # bltzal / bgezal
            imm = w & 0xFFFF
            imm = imm - 0x10000 if imm & 0x8000 else imm
            out.add((TEXT_START + off + 4 + (imm << 2)) & 0xFFFFFFFF)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(ROOT, "extern", "thps1_retail", "SLUS_008.60"))
    a = ap.parse_args()
    img = open(a.exe, "rb").read()[0x800:]
    off = CTOR_TABLE - TEXT_START
    ctors = [int.from_bytes(img[off + 4 * i:off + 4 * i + 4], "little") for i in range(CTORS)]
    calls = sorted(t for t in jal_targets(img) if GAME_END <= t < LIB_END)
    with open(OUT, "w", newline="\n") as f:
        f.write("# kind\taddr\t(platform/pc/tools/retail_refs.py, from SLUS_008.60)\n")
        f.writelines("static_ctor\t0x%08x\n" % c for c in ctors)
        f.writelines("lib_call\t0x%08x\n" % c for c in calls)
    print("%s: %d static constructors, %d library calls" % (OUT, len(ctors), len(calls)))


if __name__ == "__main__":
    main()
