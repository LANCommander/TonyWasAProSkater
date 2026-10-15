#!/usr/bin/env python3
"""Check that edited game TUs still compile to the same 32-bit code.

    python platform/pc/tools/tu_equiv.py SRC_SUBSTR... [--base DIR] [-v]

For edits that must not change what a TU does in the 32-bit build (pointer types for ints that
held pointers, a local struct replaced by the shared header's class, C turned into C++): rebuilds
each matching TU of game/tus.tsv the way build.py does, into a scratch directory, and compares its
object's disassembly (relocations by symbol, as the link sees them) with the TU's object from the
last full build (build/pc/obj, or --base). Prints SAME / DIFF per TU (-v: the differing lines).

With THPS_ARCH=x64 it builds them for x64 instead and prints what ptr64_warnings.py would say
about them (pointers cast to or from smaller integers): nothing to compare there.
"""
import argparse
import difflib
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.environ.setdefault("THPS_ARCH", "x86")
import build  # noqa: E402
import toolchain  # noqa: E402

ADDR_RE = re.compile(r"^\s*[0-9a-f]+:\s*")


def disasm(obj):
    out = subprocess.run([toolchain.tool("objdump"), "-d", "-r", "--no-show-raw-insn", obj],
                         capture_output=True, text=True, env=toolchain.env()).stdout
    lines = []
    for line in out.splitlines()[3:]:
        line = ADDR_RE.sub("", line.rstrip())
        # jump/call targets inside the object are printed as `<function+off>`: keep only the
        # offset (static helpers may be named differently, C vs C++)
        line = re.sub(r"[0-9a-f]+ <([^>+]*)(\+0x[0-9a-f]+)?>", lambda m: "<%s>" % (m.group(2) or ""),
                      line)
        line = re.sub(r"^[0-9a-f]+ <.*>:$", "<fn>:", line)
        if line.strip():
            lines.append(line)
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("srcs", nargs="+")
    ap.add_argument("--base", default=os.path.join(build.OUT, "obj"))
    ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    toolchain.add_to_path()
    ctx = build.Ctx()
    tus = [t for t in ctx.tus.values() if any(s in t["src"] for s in a.srcs)]
    if not tus:
        raise SystemExit("no TU matches")
    scratch = tempfile.mkdtemp(prefix="tu_equiv_")
    build.OUT = scratch
    flags_hash = "tu_equiv"
    bad = 0
    for tu in sorted(tus, key=lambda t: t["src"]):
        meta = build.build_tu(ctx, tu, flags_hash)
        if meta["errors"]:
            print("ERROR %s\n%s" % (tu["src"], "\n".join(meta["errors"])[:3000]))
            bad += 1
            continue
        if toolchain.X64:
            import ptr64_warnings  # noqa: E402
            _, warns = ptr64_warnings.check(tu["src"])
            print("%s %s%s" % ("WARN " if warns else "CLEAN", tu["src"],
                               "".join("\n    " + w for w in warns)))
            bad += bool(warns)
            continue
        rel = os.path.splitext(tu["src"])[0] + ".o"
        new, old = os.path.join(scratch, "obj", rel), os.path.join(a.base, rel)
        if not os.path.exists(old):
            # the base object of a TU renamed .c -> .cpp has the same path minus the extension
            print("NOBASE %s" % tu["src"])
            continue
        dn, do = disasm(new), disasm(old)
        if dn == do:
            print("SAME  %s" % tu["src"])
        else:
            bad += 1
            d = list(difflib.unified_diff(do, dn, "base", "new", lineterm="", n=2))
            print("DIFF  %s (%d lines)" % (tu["src"], sum(1 for x in d if x[:1] in "+-")))
            if a.v:
                print("\n".join(d[:200]))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
