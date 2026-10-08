#!/usr/bin/env python3
"""Find argument-count mismatches that the matching build tolerates but a native build doesn't.

On MIPS an argument nobody touches stays in its a0-a3 register, so a matching source can declare
`void Pack_KillSlot(void)` and call `Mem_Delete()` while the value still reaches Mem_Delete in a0.
On x86 the argument is simply lost. This compares, per retail function address:

  truth   the parameter count from the retail mangled name (GNU v2), when there is one
  def     the parameter count of the decompiled definition
  calls   the parameter count each caller's declaration uses

and reports every definition or declaration with fewer parameters than the truth (or than the
definition, for unnamed functions). Run after build.py (it uses build/pc/obj/*.port.c and *.rename).

    python platform/pc/tools/arity_check.py
"""
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build  # noqa: E402

OUT = build.OUT


def count_params(plist):
    """'(int, char *)' -> 2, '(void)' -> 0, '()' -> None (unknown), varargs -> None."""
    p = plist.strip()
    assert p[0] == "(" and p[-1] == ")", p
    p = p[1:-1].strip()
    if p in ("", ):
        return None
    if p == "void":
        return 0
    parts = build.portsrc._split_top(p, ",")
    if parts[-1].strip() == "...":
        return None
    return len(parts)


def aux_info(tu):
    base = build.tu_paths(tu)
    src = base + ".port.c"
    import tempfile
    aux = os.path.join(tempfile.gettempdir(), "thps_aux_%s.aux" % build.tukey(tu))
    flags = build.COMMON_FLAGS + build.C_FLAGS
    # cc1 mis-parses -aux-info paths containing an apostrophe: run beside the source instead
    aux = os.path.basename(base) + ".aux"
    subprocess.run([build.tool("gcc")] + flags + ["-fsyntax-only", "-aux-info", aux,
                                                  os.path.basename(src)],
                   capture_output=True, text=True, cwd=os.path.dirname(src))
    aux = os.path.join(os.path.dirname(src), aux)
    out = {}  # name -> (kind 'C'/'F', arity)
    if not os.path.exists(aux):
        return out
    for line in open(aux, encoding="utf-8", errors="replace"):
        m = re.match(r"/\*\s*(.*?):(\d+):([NO])([CF])\s*\*/\s*(.*)$", line)
        if not m:
            continue
        decl = re.sub(r"/\*.*?\*/", "", m.group(5)).strip()
        if "decomp" not in m.group(1).replace("\\", "/") and "obj" not in m.group(1):
            continue  # system/runtime headers
        dm = re.search(r"(\w+)\s*(\(.*\))\s*[;{]?\s*$", decl)
        if not dm:
            continue
        name, plist = dm.group(1), dm.group(2)
        try:
            n = count_params(plist)
        except Exception:  # noqa: BLE001
            continue
        if m.group(3) == "O" and m.group(4) == "C":
            n = None  # unprototyped
        prev = out.get(name)
        if prev is None or m.group(4) == "F":
            out[name] = (m.group(4), n)
    return out


def renames_of(tu):
    rn = build.tu_paths(tu) + ".rename"
    m = {}
    if os.path.exists(rn):
        for line in open(rn):
            a, b = line.split()
            m[build.strip_us(a)] = b
    return m


def addr_of(target):
    mm = re.match(r"_(?:psx|pcfn)_([0-9a-f]{8})", target)
    return int(mm.group(1), 16) if mm else None


def gnu_arity(dem):
    """Parameter count of a demangled GNU v2 name, counting `this` for methods."""
    i = dem.find("(")
    if i < 0:
        return None
    plist = dem[i:]
    j = build.portsrc._match_paren(plist, 0)
    n = count_params(plist[:j])
    if n is None and plist[:j] == "()":
        n = 0
    if n is not None and "::" in dem[:i] and not re.search(r"::operator (new|delete)", dem):
        n += 1
    return n


def main():
    os.environ["PATH"] = build.TOOLBIN + os.pathsep + os.environ["PATH"]
    ctx = build.Ctx()
    tus = [t for t in ctx.tus.values() if t["src"].endswith(".c")]
    truth = {}
    for a, (size, mangled, dem) in ctx.functions.items():
        if mangled != dem:
            truth[a] = gnu_arity(dem)
    defs, calls = {}, {}
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        results = list(ex.map(lambda t: (t, aux_info(t), renames_of(t)), tus))
    for tu, aux, rn in results:
        for name, (kind, n) in aux.items():
            tgt = rn.get(name)
            if not tgt:
                continue
            a = addr_of(tgt)
            if a is None:
                continue
            if kind == "F":
                defs[a] = (n, tu["src"], name)
            elif n is not None:
                calls.setdefault(a, []).append((n, tu["src"], name))
    problems = []
    for a in sorted(set(defs) | set(calls)):
        t = truth.get(a)
        d = defs.get(a)
        ref = t if t is not None else (d[0] if d else None)
        if ref is None:
            continue
        if d and d[0] is not None and d[0] < ref:
            problems.append("0x%08x %-40s definition has %d params, real %d   (%s)" % (
                a, ctx.functions.get(a, (0, "?", "?"))[2][:40], d[0], ref, d[1]))
        for n, src, name in calls.get(a, []):
            if n < ref:
                problems.append("0x%08x %-40s called with %d of %d params   (%s as %s)" % (
                    a, ctx.functions.get(a, (0, "?", "?"))[2][:40], n, ref, src, name))
    print("\n".join(problems))
    print("%d problems" % len(problems), file=sys.stderr)


if __name__ == "__main__":
    main()
