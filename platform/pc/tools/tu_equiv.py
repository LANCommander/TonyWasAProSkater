#!/usr/bin/env python3
"""Check that edited game sources still compile to the same 32-bit code, function by function.

    python platform/pc/tools/tu_equiv.py SRC_SUBSTR... [--base DIR] [--no-ipa] [-v]
    python platform/pc/tools/tu_equiv.py --save-base DIR [--no-ipa]

For edits that must not change what a function does in the 32-bit build (pointer types for ints
that held pointers, a private struct replaced by the shared header's class, C turned into C++,
functions moved between module files): rebuilds the matching game TUs the way build.py does,
into a scratch directory, and compares each of their functions with the same function (by the
symbol the link sees: pcfn_<retail address>) in the base build: by default the last full build
(build/pc), whose compiler assembly (obj/**/*.fix.s) and renames (*.rename) it keeps. A function
may have been in another TU there.

The comparison is of canonical assembly: local labels numbered per function, constants and jump
tables inlined where they are used, calls named by retail address, vtable thunks by class and slot,
no debug directives. Prints SAME/DIFF per function (-v: the differing lines), NEW for a function
the base doesn't have, NODEF for a header comment no definition matched and UNRES for a symbol
nothing resolves (and the base resolved).

--no-ipa: compile without inter-procedural optimisation, so that a function's code can't depend
on the other functions of its TU; needed when functions move between TUs. The base must then be
built the same way first (--save-base DIR --no-ipa, before the edit).

With THPS_ARCH=x64 it builds them for x64 instead and prints what ptr64_warnings.py would say
about them (pointers cast to or from smaller integers): nothing to compare there.
"""
import argparse
import concurrent.futures as cf
import difflib
import json
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.environ.setdefault("THPS_ARCH", "x86")
import build  # noqa: E402
import toolchain  # noqa: E402

DEFAULT_BASE = build.OUT  # the last full build

NO_IPA = ["-fno-ipa-pure-const", "-fno-ipa-reference", "-fno-ipa-reference-addressable",
          "-fno-ipa-modref", "-fno-inline-functions-called-once", "-fno-ipa-icf", "-fno-ipa-sra",
          "-fno-ipa-cp", "-fno-ipa-ra", "-fno-ipa-vrp", "-fno-ipa-bit-cp", "-fno-ipa-profile",
          "-fno-ipa-stack-alignment", "-fno-inline-small-functions", "-fno-toplevel-reorder"]


def build_tus(out, subs):
    """Build the game TUs whose path contains one of subs (all if none) into out/obj. -> metas"""
    build.OUT = out
    ctx = build.Ctx()
    tus = [t for t in ctx.tus.values() if not subs or any(s in t["src"] for s in subs)]
    if not tus:
        raise SystemExit("no TU matches")
    stamp = "tu_equiv" + "".join(open(os.path.join(HERE, f)).read()
                                 for f in ("build.py", "portsrc.py")) + " ".join(build.COMMON_FLAGS)
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        return list(ex.map(lambda t: build.build_tu(ctx, t, stamp), tus))


# --------------------------------------------------------------------------------------------
# canonical assembly

SKIP_RE = re.compile(r"^\s*(\.(loc|cfi_\w+|file|align|p2align|balign|globl|def|scl|endef|ident|"
                     r"text|data|bss|section|type|size|weak|linkonce|lcomm|comm|seh_\w+)\b|"
                     r"(LVL|LFB|LFE|LBB|LBE|LBI|LVU|LM|Ltext|Letext|LCOLDB|LCOLDE|LHOTB|LHOTE)\d*:)")
LABEL_RE = re.compile(r"^([A-Za-z_.$@?][\w.$@?]*):\s*$")
IDENT_RE = re.compile(r"(?<![%\w.@])([A-Za-z_.@?][\w.$@?]*)")
SECTION_RE = re.compile(r"^\s*(\.text|\.data|\.bss|\.section\s+([^\s,]+))")
DATA_DIRECTIVE_RE = re.compile(r"^\s*\.(long|word|short|byte|ascii|asciz|string|space|zero|quad|"
                               r"value|uleb128|sleb128|single|double|float|skip|fill|rva|secrel32)\b")


def parse_asm(path):
    """-> (functions {label: [lines]}, data {label: [lines]}): the TU's local data only (a
    global's storage is game/data's or the TU's own business, not a function's code)."""
    funcs, data = {}, {}
    section, cur_fn, cur_data = ".text", None, None
    globls = set()
    for raw in open(path, encoding="utf-8", errors="replace"):
        line = raw.rstrip("\n")
        gm = re.match(r"^\s*\.globl\s+(\S+)", line)
        if gm:
            globls.add(gm.group(1))
        sm = SECTION_RE.match(line)
        if sm:
            section = sm.group(2) or sm.group(1)
            cur_data = None
            continue
        if section.startswith(".debug") or section.startswith(".rdata$zzz"):
            continue
        lm = LABEL_RE.match(line)
        if section == ".text":
            if lm and not re.match(r"^L\w*\d+$", lm.group(1)):
                cur_fn = lm.group(1)
                funcs[cur_fn] = []
                continue
            if cur_fn is not None:
                funcs[cur_fn].append(line)
            continue
        if lm:
            cur_data = lm.group(1)
            data[cur_data] = []
            continue
        if cur_data is not None and DATA_DIRECTIVE_RE.match(line):
            data[cur_data].append(line.strip())
    for g in globls:
        data.pop(g, None)
    return funcs, data


def canon_symbol(s):
    s = re.sub(r"_a\d+$", "", s)  # build.py's aliases of one renamed symbol
    # a call by retail address, the function defined here, a constructor defined here (called
    # natively; the build wraps it at the retail address): one function
    s = re.sub(r"^_(pcfn|psx|pcctor)_([0-9a-f]{8})$", r"FN_\2", s)
    # a base-object destructor defined here, called natively, or through the in_chrg-0 wrapper
    s = re.sub(r"^_pcdtor(call)?_([0-9a-f]{8})$", r"DT2_\2", s)
    s = re.sub(r"^_pcvt_\w+?_(C\w+?_\d+_[dm])$", r"VT_\1", s)  # thunks are per TU
    s = re.sub(r"^_pcvptroff_\w+?_(C\w+)$", r"VO_\1", s)
    s = re.sub(r"\.\d+$", "", s)  # numbered statics: count.0
    return s


class Canon:
    def __init__(self, funcs, data, renames):
        self.funcs, self.data, self.renames = funcs, data, renames

    def sym(self, s):
        if s not in self.renames and "C2E" in s:
            # a base-object constructor defined in this TU: native, called directly; the same
            # code as the complete-object one the build puts at the retail address
            m = re.match(r"^_pcctor_([0-9a-f]{8})$", self.renames.get(s.replace("C2E", "C1E"), ""))
            if m:
                return "FN_" + m.group(1)
        return canon_symbol(self.renames.get(s, s))

    def function(self, name):
        labels = {}
        out, pending, seen = [], [], set()

        def tok(m):
            s = m.group(1)
            if re.match(r"^L\d+$", s):
                if s not in labels:
                    labels[s] = "L%d" % len(labels)
                    if s in self.data:
                        pending.append(s)
                return labels[s]
            if re.match(r"^LC\d+$", s):
                return "C{%s}" % " ; ".join(self.data.get(s, ["?"]))
            if s in self.data and s not in self.funcs and s not in seen:
                seen.add(s)
                pending.append(s)
            return self.sym(s)

        for line in self.funcs[name]:
            if line.strip() and not SKIP_RE.match(line):
                out.append(IDENT_RE.sub(tok, line.strip()))
        # jump tables and the TU's local data the function names, once each
        done = set()
        while pending:
            s = pending.pop(0)
            if s in done:
                continue
            done.add(s)
            out.append("DATA %s:" % labels.get(s, self.sym(s)))
            for d in self.data.get(s, []):
                out.append("  " + IDENT_RE.sub(tok, d))
        return out


def tu_functions(objdir, rel):
    """{canonical name: canonical lines} of the TU built at objdir/obj/<rel>.*"""
    base = os.path.join(objdir, "obj", rel)
    renames = {}
    if os.path.exists(base + ".rename"):
        for line in open(base + ".rename"):
            p = line.split()
            if len(p) == 2:
                renames[p[0]] = p[1]
    funcs, data = parse_asm(base + ".fix.s")
    c = Canon(funcs, data, renames)
    return dict((c.sym(f), c.function(f)) for f in funcs)


def tus_in(objdir):
    out, root = [], os.path.join(objdir, "obj")
    for dp, _, files in os.walk(root):
        for f in files:
            if f.endswith(".fix.s"):
                out.append(os.path.relpath(os.path.join(dp, f[:-6]), root).replace("\\", "/"))
    return out


def compare(basedir, newdir, rels, verbose):
    base = {}
    unres = set()
    brels = tus_in(basedir)
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        for rel, fns in zip(brels, ex.map(lambda r: tu_functions(basedir, r), brels)):
            for k, v in fns.items():
                base.setdefault(k, []).append((rel, v))
            mp = os.path.join(basedir, "obj", rel + ".json")
            if os.path.exists(mp):
                unres.update(json.load(open(mp)).get("unresolved", []))
    same = diff = other = 0
    for rel in sorted(rels):
        mp = os.path.join(newdir, "obj", rel + ".json")
        meta = json.load(open(mp)) if os.path.exists(mp) else {}
        for h in meta.get("undefined_headers", []):
            print("NODEF %s %s" % (rel, h))
            other += 1
        for u in meta.get("unresolved", []):
            if u not in unres:
                print("UNRES %s %s" % (rel, u))
                other += 1
        for name, lines in sorted(tu_functions(newdir, rel).items()):
            cands = base.get(name)
            if not cands:
                print("NEW   %s %s" % (rel, name))
                other += 1
            elif any(lines == c for _, c in cands):
                same += 1
                if verbose:
                    print("SAME  %s %s" % (rel, name))
            else:
                diff += 1
                brel, blines = cands[0]
                d = list(difflib.unified_diff(blines, lines, brel, rel, lineterm="", n=2))
                print("DIFF  %s %s (base %s, %d lines)" % (rel, name, brel,
                                                           sum(1 for x in d if x[:1] in "+-")))
                if verbose:
                    print("\n".join(d[:200]))
    print("%d functions the same, %d differ, %d other findings" % (same, diff, other))
    return 1 if diff or other else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("srcs", nargs="*")
    ap.add_argument("--base", default=None)
    ap.add_argument("--save-base", default=None, metavar="DIR")
    ap.add_argument("--no-ipa", action="store_true")
    ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    toolchain.add_to_path()
    if a.no_ipa:
        build.COMMON_FLAGS += NO_IPA
    if a.save_base:
        metas = build_tus(os.path.abspath(a.save_base), a.srcs)
        bad = [m for m in metas if m["errors"]]
        for m in bad:
            print("ERROR %s\n%s" % (m["src"], "\n".join(m["errors"])[:3000]))
        print("%d TUs built into %s, %d errors" % (len(metas), a.save_base, len(bad)))
        return 1 if bad else 0
    if not a.srcs:
        raise SystemExit("which sources? (substrings of game/src paths)")
    scratch = tempfile.mkdtemp(prefix="tu_equiv_")
    metas = build_tus(scratch, a.srcs)
    bad = 0
    for m in metas:
        if m["errors"]:
            print("ERROR %s\n%s" % (m["src"], "\n".join(m["errors"])[:3000]))
            bad += 1
    if toolchain.X64:
        import ptr64_warnings  # noqa: E402
        for m in metas:
            _, warns = ptr64_warnings.check(m["src"])
            print("%s %s%s" % ("WARN " if warns else "CLEAN", m["src"],
                               "".join("\n    " + w for w in warns)))
            bad += bool(warns)
        return 1 if bad else 0
    if bad:
        return 1
    rels = [os.path.splitext(m["src"])[0] for m in metas]
    return compare(os.path.abspath(a.base or DEFAULT_BASE), scratch, rels, a.v)


if __name__ == "__main__":
    sys.exit(main())
