#!/usr/bin/env python3
"""The retail image the port needs at runtime, without its code: build/pc/gen/image.bin.

The port keeps the PS1's data at its retail addresses (initialised globals, tables, strings,
vtables, the movie table...), so the 0x80010000.. image is copied into PS1 RAM at startup. Its
code is not needed: every function is C (decompiled, rewritten or HLE) and the trampolines go
over the function addresses. This writes the image with every instruction word cleared:
  * game functions (decomp/functions.tsv): their whole extent;
  * the hand-written assembly and the Psy-Q libraries after the game code: all of it.
Data that lives between game functions in .text (the memory-layout table at 0x80071fa4) stays.
The result is embedded in thps.exe (link.py: image.s). Prints what stayed in .text.

    python platform/pc/tools/embed_image.py <exe> <out.bin>
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mips2c  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
TEXT_START = 0x80010000
GAME_END = 0x800858AC  # the hand-written assembly and then the libraries follow the game code
LIB_END = 0x8009D0E0  # first byte of .rodata


def main():
    exe_path, out = sys.argv[1:3]
    exe = mips2c.Exe(exe_path)
    code = set()
    for line in open(os.path.join(ROOT, "decomp", "functions.tsv"), encoding="utf-8"):
        p = line.split("\t")
        if p[0].startswith("0x") and not p[2].startswith("D_"):  # D_: data in .text
            code.update(range(int(p[0], 16), int(p[0], 16) + int(p[1]), 4))
    # hand-written assembly and the Psy-Q libraries: all code (no data lives there)
    code.update(range(GAME_END, LIB_END, 4))
    img = bytearray(exe.data)
    cleared = 0
    for a in code:
        if TEXT_START <= a < LIB_END:
            img[a - exe.base:a - exe.base + 4] = b"\0\0\0\0"
            cleared += 4
    kept = [a for a in range(TEXT_START, LIB_END, 4)
            if struct.unpack_from("<I", img, a - exe.base)[0]]
    runs, start, prev = [], None, None
    for a in kept:
        if start is None or a != prev + 4:
            if start is not None:
                runs.append((start, prev + 4))
            start = a
        prev = a
    if start is not None:
        runs.append((start, prev + 4))
    with open(out + ".tmp", "wb") as f:
        f.write(img)
    os.replace(out + ".tmp", out)
    print("embed_image: %d KB of code cleared, %d bytes of .text kept in %d runs" % (
        cleared >> 10, 4 * len(kept), len(runs)))
    for s, e in runs:
        if e - s >= 16:
            print("  kept %08x-%08x (%d bytes)" % (s, e, e - s))


if __name__ == "__main__":
    main()
