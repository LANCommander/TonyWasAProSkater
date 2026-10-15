#!/usr/bin/env python3
"""The x64 compiler's warnings about pointers and integers of different sizes, per game TU.

    python platform/pc/tools/ptr64_warnings.py [--json OUT] [SUBSTR...]

Recompiles (syntax only) the transformed sources of the x64 build (build/pc64/obj/**/*.port.c*,
so build it first) with the build's flags and the warnings it turns off: casts between pointers
and smaller integers, int <-> pointer conversions, and what -fpermissive lets C++ get away with
(`cast from 'char*' to 'int' loses precision`). Each one is a place where a pointer may be cut
to 32 bits.
"""
import argparse
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if __name__ == "__main__":
    os.environ["THPS_ARCH"] = "x64"
sys.path.insert(0, HERE)
import build  # noqa: E402
import toolchain  # noqa: E402

WANT = re.compile(r"(int-to-pointer-cast|pointer-to-int-cast|int-conversion|loses precision|"
                  r"invalid conversion from '[^']*\*[^']*' to '(int|unsigned int|long|"
                  r"long unsigned int|short)'|invalid conversion from '(int|unsigned int|long|"
                  r"long unsigned int)' to '[^']*\*')")
WARN = ["-Wpointer-to-int-cast", "-Wint-to-pointer-cast", "-Wint-conversion"]


def check(src):
    base = os.path.join(build.OUT, "obj", os.path.splitext(src)[0])
    port = base + (".port.cpp" if src.endswith(".cpp") else ".port.c")
    if not os.path.exists(port):
        return src, ["(no %s: build first)" % port]
    cxx = src.endswith(".cpp")
    flags = [f for f in build.COMMON_FLAGS if f != "-w"] + \
        (build.CXX_FLAGS if cxx else build.C_FLAGS) + WARN
    r = subprocess.run([toolchain.tool("g++" if cxx else "gcc")] + flags +
                       ["-fsyntax-only", port], capture_output=True, text=True,
                       env=toolchain.env())
    out = []
    for line in r.stderr.splitlines():
        m = re.match(r"^(.*?):(\d+):\d+: (?:warning|error): (.*)$", line)
        if m and WANT.search(m.group(3)) and "/include/" not in m.group(1).replace("\\", "/"):
            out.append("%s:%s: %s" % (os.path.basename(m.group(1)), m.group(2), m.group(3)))
    return src, sorted(set(out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("only", nargs="*")
    ap.add_argument("--json")
    a = ap.parse_args()
    tus = [s for s in build.game_sources() if not a.only or any(o in s for o in a.only)]
    res = {}
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        for src, warns in ex.map(check, tus):
            if warns:
                res[src] = warns
    for src in sorted(res):
        for w in res[src]:
            print("%s: %s" % (src, w))
    print("%d warnings in %d TUs" % (sum(len(v) for v in res.values()), len(res)))
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
