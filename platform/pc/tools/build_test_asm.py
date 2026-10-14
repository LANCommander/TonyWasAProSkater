#!/usr/bin/env python3
"""Build build/pc/test_asm.exe, the differential test of C rewrites of retail MIPS (see
platform/pc/tests/test_asm.c).

    python platform/pc/tools/build_test_asm.py [--src a.c b.c ...] [--lib x.c ...] [--out NAME]
                                               [--run [snapshot dirs/files...]] [-v] [--only ADDR]

What it tests:
  * decomp/reimpl.tsv rows whose verify column is `test_asm` (hand-written assembly as C), or the
    sources given with --src (each starts with the header `/* <name> (0x<addr>) ...`), and
  * the Psy-Q library functions defined in the runtime files listed in LIB_SOURCES (or --lib), by
    their names in build/psyq_names.tsv (PC_<name> as well: the port's renamed libgte).
--out NAME builds build/pc/NAME.exe (default test_asm), so several can be built side by side.
--run runs it on those snapshots (default: all of build/pc/snaps); -v and --only go to the test.

A function returns a value unless its definition starts with `void`. A comment
`test_asm: ignore 0x1f8003c4 ...` in a source lists words the C may leave different (scratchpad
temporaries nothing reads afterwards; say why next to it); `test_asm: kseg 0x800d....` lists
pointer words the C keeps in KSEG0 where retail strips bit 31 (compared with bit 31 set).
`test_asm: gte_out ir1 ir2 ir3 mac1 ...` (or `none`) limits the GTE data registers compared to
those (C on pc_geom.h has no registers: these are the results its callers read); the control
registers but FLAG are always compared (`flag` adds it). `test_asm: lastvec ir|v0|out` marks
a function on M3DMATHS' last vector (see lastvec() below); `test_asm: colourcode` one on
M3dAsm_ColourCode (decomp/src/m3dasm/processpolys.h), loaded from and stored back to RGBC.
Globals and functions the sources reference by name come from their .syms.json files (as in the
port build), as absolute symbols in a generated linker script.
"""
import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PC = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(PC))
DECOMP = os.path.join(ROOT, "decomp")
TOOLBIN = os.environ.get("THPS_MINGW", r"C:\msys64\mingw32\bin")

LIB_SOURCES = ["src/pc_libgte.c"]

DEF_RE = r"^(?:static\s+)?([A-Za-z_][\w \t\*]*?)\b%s\s*\(([^)]*)\)\s*\{"
HEADER_RE = r"/\*\s*(\S+)\s+\((0x[0-9a-fA-F]+)\)"


def read_tsv(path):
    for line in open(path, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        yield line.rstrip("\n").split("\t")


def returns_value(text, name):
    m = re.search(DEF_RE % re.escape(name), text, re.M)
    if not m:
        raise SystemExit("no definition of %s" % name)
    return not re.match(r"^(static\s+)?void\b", m.group(1).strip() + " ") or "*" in m.group(1)


def addr_list(text, what):
    out = []
    for m in re.finditer(r"test_asm:\s*%s((?:\s+0x[0-9a-fA-F]+)+)" % what, text):
        out += [int(x, 16) for x in m.group(1).split()]
    return out


# GTE data registers by name, for `test_asm: gte_out` (63: FLAG, control register 31)
GTE_REGS = dict([("v0xy", 0), ("v0z", 1), ("rgbc", 6), ("otz", 7), ("ir0", 8), ("ir1", 9),
                 ("ir2", 10), ("ir3", 11), ("sxy0", 12), ("sxy1", 13), ("sxy2", 14), ("sz0", 16),
                 ("sz1", 17), ("sz2", 18), ("sz3", 19), ("rgb0", 20), ("rgb1", 21), ("rgb2", 22),
                 ("mac0", 24), ("mac1", 25), ("mac2", 26), ("mac3", 27), ("lzcs", 30),
                 ("lzcr", 31), ("flag", 63)])


def gte_cmp(text):
    """All 64 registers, or with `test_asm: gte_out r ...` (or `none`): the control registers
    but FLAG, and the named ones."""
    m = re.search(r"test_asm:\s*gte_out((?:[ \t]+\w+)+)", text)
    if not m:
        return (1 << 64) - 1
    bits = ((1 << 31) - 1) << 32
    for r in m.group(1).split():
        if r == "none":
            continue
        if r not in GTE_REGS:
            raise SystemExit("test_asm: gte_out: unknown register %s" % r)
        bits |= 1 << GTE_REGS[r]
    return bits


LASTVEC = {"ir": 1, "v0": 2, "out": 3}


def lastvec(text):
    """`test_asm: lastvec ir|v0|out`: the function works on M3DMATHS' last vector
    (decomp/src/m3dasm/lastvec.h), which retail kept in the GTE: the harness loads it from the
    snapshot's IR1..3 (v0: V0) and MAC1..3, and stores it back to compare with retail's."""
    m = re.search(r"test_asm:\s*lastvec\s+(\w+)", text)
    if not m:
        return 0
    if m.group(1) not in LASTVEC:
        raise SystemExit("test_asm: lastvec: %s?" % m.group(1))
    return LASTVEC[m.group(1)]


def impl(addr, name, text):
    lv = lastvec(text)
    cc = 1 if re.search(r"test_asm:\s*colourcode", text) else 0
    cmp = gte_cmp(text)
    if lv:
        cmp = (((1 << 31) - 1) << 32) | (7 << 9) | (7 << 25)
    if cc:  # M3dAsm_ColourCode (processpolys.h) is retail's RGBC
        cmp = (((1 << 31) - 1) << 32) | (1 << 6)
    return (addr, name, returns_value(text, name), addr_list(text, "ignore"),
            addr_list(text, "kseg"), cmp, lv, cc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", nargs="*")
    ap.add_argument("--src", nargs="*")
    ap.add_argument("--lib", nargs="*")
    ap.add_argument("--out", default="test_asm")
    a, rest = ap.parse_known_args()  # the rest (-v, --only ...) goes to the test with --run
    out = os.path.join(ROOT, "build", "pc", a.out)
    os.makedirs(out, exist_ok=True)
    impls, srcs, syms = [], [], {}
    if a.src is not None:
        rows = []
        for src in a.src:
            m = re.match(HEADER_RE, open(src, encoding="utf-8").readline())
            if not m:
                raise SystemExit("%s: no `/* <name> (0x<addr>)` header" % src)
            rows.append([os.path.abspath(src), m.group(2), "0", "test_asm", m.group(1)])
    else:
        rows = [[os.path.join(DECOMP, p[0])] + p[1:]
                for p in read_tsv(os.path.join(DECOMP, "reimpl.tsv"))]
    for p in rows:
        if len(p) < 5 or p[3] != "test_asm":
            continue
        src = p[0]
        text = open(src, encoding="utf-8").read()
        if src not in srcs:
            srcs.append(src)
            sj = os.path.splitext(src)[0] + ".syms.json"
            if os.path.exists(sj):
                for k, v in json.load(open(sj)).items():
                    if k != "gp" and not k.startswith("."):
                        syms[k] = int(v, 16)
        impls.append(impl(int(p[1], 16), p[4], text))
    psyq = {}
    for p in read_tsv(os.path.join(ROOT, "build", "psyq_names.tsv")):
        psyq.setdefault(p[1], []).append(int(p[0], 16))
    for rel in (a.lib if a.lib is not None else LIB_SOURCES):
        src = os.path.join(PC, rel) if a.lib is None else os.path.abspath(rel)
        if not os.path.exists(src):
            continue
        srcs.append(src)
        text = open(src, encoding="utf-8").read()
        for m in re.finditer(r"^(?!static)([A-Za-z_][\w \t\*]*?)\b(\w+)\s*\([^;{]*\)\s*\{",
                             text, re.M):
            name = m.group(2)
            for addr in psyq.get(name[3:] if name.startswith("PC_") else name, []):
                impls.append(impl(addr, m.group(2), text))
    defined = {i[1] for i in impls}
    h = ["/* generated by tools/build_test_asm.py */"]
    h += ["extern char %s[];" % n for n in sorted(defined)]
    h.append("static const AsmImpl asm_impls[] = {")
    for addr, name, ret, ign, ks, cmp, lv, cc in sorted(impls):
        h.append('\t{0x%08xu, (void *)%s, "%s", %d, {%s}, {%s}, 0x%016xull, %d, %d},' % (
            addr, name, name, int(ret), ", ".join("0x%08xu" % x for x in ign[:8]) or "0",
            ", ".join("0x%08xu" % x for x in ks[:4]) or "0", cmp, lv, cc))
    h.append("\t{0, 0, 0, 0, {0}, {0}, 0, 0, 0}\n};")
    open(os.path.join(out, "test_asm_impls.h"), "w").write("\n".join(h) + "\n")
    ld = "".join("_%s = 0x%08x;\n" % (k, v) for k, v in sorted(syms.items()) if k not in defined)
    open(os.path.join(out, "syms.ld"), "w").write(ld)
    env = dict(os.environ, PATH=TOOLBIN + os.pathsep + os.environ["PATH"])
    gcc = os.path.join(TOOLBIN, "gcc.exe")
    flags = ["-m32", "-O1", "-g", "-std=gnu99", "-fno-strict-aliasing", "-fwrapv",
             "-fno-delete-null-pointer-checks", "-fno-builtin", "-funsigned-char", "-w",
             "-DPC_PORT=1", "-I", os.path.join(PC, "include", "psyq"), "-I",
             os.path.join(PC, "include"), "-I", os.path.join(DECOMP, "include")]
    exe = out + ".exe"
    cmd = [gcc] + flags + ["-DMREF_GTE", "-DPC_MIPS=1", "-I", out, "-I", os.path.join(PC, "tests"),
                           os.path.join(PC, "tests", "test_asm.c"),
                           os.path.join(PC, "tests", "mips_ref.c"),
                           os.path.join(PC, "src", "pc_gte.c"),
                           os.path.join(PC, "src", "pc_geom.c")] + srcs + [
        "-o", exe, "-Wl,--large-address-aware", "-Wl,--disable-dynamicbase",
        "-Wl," + os.path.join(out, "syms.ld").replace("\\", "/")]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit("test_asm: build failed")
    print("built %s: %d functions" % (exe, len(impls)))
    if a.run is not None:
        return subprocess.run([exe] + rest + a.run, cwd=ROOT).returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
