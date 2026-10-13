#!/usr/bin/env python3
"""Check shared class headers against the layouts the TUs' own local declarations had.

    python platform/pc/tools/header_check.py decomp/include/game/bit.h [...] [--egcs] [--class C]

Compiles a test TU that includes the headers (as the port does: project headers inlined, egcs vptrs
inserted by portsrc), flattens every class they define (layouts.py), and compares each with every
TU's version of that class in build/layouts_baseline.json (`layouts.py snapshot`). Per class: how many
TUs agree, and for each TU that doesn't, its first conflicting leaf (missing / other size = error,
other signedness or pointer-ness = type warning). A class name some TUs use for a different class
(three unrelated `CItem`s exist) shows up as a group of TUs that all conflict.

--egcs also compiles the test TU with the matching compiler (Psy-Q cc1plus), which the headers must
keep building with since byte-exact TUs include them too.
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build  # noqa: E402
import layouts  # noqa: E402
import portsrc  # noqa: E402


def compile_layouts(headers):
    os.environ["PATH"] = build.TOOLBIN + os.pathsep + os.environ["PATH"]
    tmp = tempfile.mkdtemp(prefix="hdrchk_")
    src = os.path.join(tmp, "hdrchk.cpp")
    text = "".join('#include "%s"\n' % os.path.abspath(h).replace("\\", "/") for h in headers)
    text, _ = build.expand_includes(text, src)
    text, _ = portsrc.transform_cxx(portsrc.translate_asm(text), {}, "hdrchk")
    open(src, "w", encoding="utf-8").write(text)
    obj = os.path.join(tmp, "hdrchk.o")
    r = subprocess.run([build.tool("g++")] + build.COMMON_FLAGS + build.CXX_FLAGS + [
        "-fno-merge-debug-strings", "-gdwarf-4", "-fno-eliminate-unused-debug-types", "-c", src,
        "-o", obj], capture_output=True, text=True)
    if r.returncode:
        sys.exit("g++ failed:\n" + r.stderr[-4000:])
    r = subprocess.run([build.tool("objdump"), "--dwarf=info", obj], capture_output=True,
                       text=True, errors="replace")
    dies = layouts.parse_dwarf(r.stdout)
    # only classes the headers define (not the SDK's)
    hdrs = {os.path.normcase(os.path.abspath(h)) for h in headers}
    return layouts.Layouts(dies).classes(), hdrs


def egcs_check(headers):
    tmp = os.path.join(build.DECOMP, "build")
    os.makedirs(tmp, exist_ok=True)
    src = os.path.join(tmp, "hdrchk_%d.cpp" % os.getpid())
    open(src, "w").write("".join('#include "%s"\n' % os.path.abspath(h).replace("\\", "/")
                                 for h in headers) + "int hdrchk_dummy;\n")
    r = subprocess.run(["node", "tools/compile.mjs", src, "-o", src[:-4] + ".s"], cwd=build.DECOMP,
                       capture_output=True, text=True)
    for f in (src, src[:-4] + ".s"):
        if os.path.exists(f):
            os.remove(f)
    return r.returncode == 0, (r.stdout + r.stderr)[-3000:]


def first_conflict(hdr_leaves, tu_leaves):
    have = {}
    for o, s, k, n, pad in hdr_leaves:
        if not pad:
            have.setdefault(o, []).append((s, k, n))
    err = warn = None
    for o, s, k, n, pad in tu_leaves:
        if pad:
            continue
        same = [c for c in have.get(o, []) if c[0] == s]
        if not same:
            got = ", ".join("%s %s(%d)" % (c[2], c[1], c[0]) for c in have.get(o, [])) or "nothing"
            err = err or "+0x%x %s %s(%d) -> header has %s" % (o, n, k, s, got)
        elif not any(c[1] == k for c in same) and warn is None:
            warn = "+0x%x %s %s -> header %s %s" % (o, n, k, same[0][2], same[0][1])
    return err, warn


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("headers", nargs="+")
    ap.add_argument("--egcs", action="store_true")
    ap.add_argument("--class", dest="cls", action="append")
    ap.add_argument("--baseline", default=os.path.join(build.ROOT, "build", "layouts_baseline.json"))
    a = ap.parse_args()
    classes, _ = compile_layouts(a.headers)
    base = json.load(open(a.baseline))
    # which classes do these headers define? those whose name appears in a class/struct head
    import re
    defined = set()
    for h in a.headers:
        defined |= set(re.findall(r"^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+(\w+)\s*(?::[^{;]*)?\{",
                                  open(h, encoding="utf-8").read(), re.M))
    bad = 0
    for cls in sorted(defined):
        if a.cls and cls not in a.cls:
            continue
        if cls not in classes:
            print("%s: not in the compiled output (template or unused?)" % cls)
            continue
        hl = classes[cls]["leaves"]
        ok, errs, warns = [], [], []
        for src, cs in sorted(base.items()):
            if cls not in cs:
                continue
            e, w = first_conflict(hl, cs[cls]["leaves"])
            if e:
                errs.append((src, e))
            elif w:
                warns.append((src, w))
            else:
                ok.append(src)
        print("%s (size 0x%x): %d TUs agree, %d type warnings, %d conflicts" % (
            cls, classes[cls]["size"], len(ok), len(warns), len(errs)))
        for src, e in errs:
            print("  CONFLICT %s: %s" % (src, e))
        for src, w in warns:
            print("  warn     %s: %s" % (src, w))
        bad += len(errs)
    if a.egcs:
        ok, out = egcs_check(a.headers)
        print("egcs cc1plus: %s" % ("ok" if ok else "FAILED\n" + out))
        bad += 0 if ok else 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
