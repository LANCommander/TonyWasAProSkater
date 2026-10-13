#!/usr/bin/env python3
"""Find aggregate-return ABI mismatches in the linked port.

On MIPS (egcs) `T f(a)` and `void f(T *, a)` are the same call, and the decomp uses both forms for
one function. SysV i386 has the callee pop the hidden pointer (`ret $4`) and by-value callers
re-adjust (`sub $4,%esp`), so a mixed pair leaves %esp 4 bytes off and later stack loads read the
wrong slot. build.py's caller_pops_sret makes game code caller-pops throughout; this checks that
nothing slipped through:

  - a game function (`pcfn_<addr>`) that still ends in `ret $0x4`;
  - a call into a `ret $0x4` function that isn't followed by `sub $0x4,%esp`;
  - a call into any other function that is.

    python platform/pc/tools/sret_check.py [path/to/thps.exe]
"""
import collections
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import toolchain  # noqa: E402
EXE = os.path.join(toolchain.OUT, toolchain.EXE_NAME)
OBJDUMP = toolchain.tool("objdump")

FUNC_RE = re.compile(r"^[0-9a-f]+ <(.+)>:")
INSN_RE = re.compile(r"^\s+[0-9a-f]+:\s+(.*)")
CALL_RE = re.compile(r"call\s+[0-9a-f]+ <(.+)>")
GAME_RE = re.compile(r"^_?(?:psx|pcfn)_([0-9a-f]{8})$")  # PE names have a '_', ELF's don't


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else EXE
    dis = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", exe], capture_output=True,
                         text=True).stdout
    insns, sret, func = [], set(), None
    for line in dis.splitlines():
        m = FUNC_RE.match(line)
        if m:
            func = m.group(1)
            continue
        m = INSN_RE.match(line)
        if m:
            ins = m.group(1).strip()
            insns.append((func, ins))
            if re.match(r"ret\s+\$0x4\b", ins):
                sret.add(func)
    game_sret = sorted(f for f in sret if f.startswith("_pcfn_"))
    missing, extra = collections.Counter(), collections.Counter()
    for i, (f, ins) in enumerate(insns):
        m = CALL_RE.match(ins)
        if not m:
            continue
        tgt = m.group(1)
        g = GAME_RE.match(tgt)
        if g:
            tgt = "_pcfn_" + g.group(1)
        fixed = i + 1 < len(insns) and re.match(r"sub\s+\$0x4,%esp", insns[i + 1][1])
        if tgt in sret and not fixed and (g or f.startswith("_pcfn_")):
            missing[(f, tgt)] += 1
        elif tgt not in sret and fixed and (g or f.startswith("_pcfn_")):
            extra[(f, tgt)] += 1
    for f in game_sret:
        print("callee pops its return pointer: %s" % f)
    for (f, t), n in sorted(missing.items()):
        print("caller doesn't re-adjust after %s: %s (x%d)" % (t, f, n))
    for (f, t), n in sorted(extra.items()):
        print("caller re-adjusts after non-popping %s: %s (x%d)" % (t, f, n))
    bad = len(game_sret) + len(missing) + len(extra)
    print("%d problem(s)" % bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
