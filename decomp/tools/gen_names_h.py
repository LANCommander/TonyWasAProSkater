#!/usr/bin/env python3
"""Readable names for the C sources: generate include/names.h, a list of
    #define Mess_SetTextJustify Mess_SetTextJustify__FUc
aliases, so a .c file can call g++-mangled functions by the name the developers wrote while the compiler
still emits the mangled symbol (the code and the syms.json keys don't change).

  free function  Name__F...        ->  Name
  method         Meth__<n>Class... ->  Class_Meth     (const methods too)
  constructor    __<n>Class...     ->  Class_ctor

An alias is left out (the source keeps the mangled name) when it would be ambiguous: an overload, a second
constructor, the same alias for two symbols, or an alias that's already an identifier in some .c source or
in the Psy-Q SDK headers. Destructors (_._) and operators aren't identifiers / aren't aliased.

  python tools/gen_names_h.py            regenerate include/names.h (aliases for symbols the .c files use)
  python tools/gen_names_h.py --apply    ... and rewrite the .c sources to use the aliases (line 1, the
                                         salvage header, and string literals are left alone; adds
                                         #include "names.h" as line 2)
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from demangle import demangle  # noqa: E402

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PSYQ_INC = os.environ.get("PSYQ_INC", r"D:\Tools\psyq4.5\include")
HEADER = os.path.join(ROOT, "include", "names.h")
MANGLED = re.compile(r"\b(?:[A-Za-z_]\w*?__(?:C?\d+[A-Za-z_]\w*|F\w*)|__\d+[A-Za-z_]\w*)\b")
IDENT = re.compile(r"\b[A-Za-z_]\w*\b")
STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')
SIMPLE = re.compile(r"^[A-Za-z_]\w*$")


def c_sources():
    return sorted(f.replace("\\", "/") for f in glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True))


def universe():
    """The retail functions (functions.tsv), to detect overloads. Not the demo SYM or stale syms.json
    keys: those carry other signatures for the same THPS1 function and would look like overloads."""
    names = set()
    for l in open(os.path.join(ROOT, "functions.tsv"), encoding="utf-8"):
        if l.startswith("0x"):
            names.add(l.split("\t")[2])
    return names


def alias_of(written):
    """'CBruce::Vibrate(int, int, int)' -> 'CBruce_Vibrate'; None if not aliasable."""
    m = re.match(r"^([\w:]+)\((.*)\)( const)?$", written)
    if not m or "<" in written:
        return None
    parts = m.group(1).split("::")
    if parts[-1].startswith(("FUN_", "DAT_")):       # placeholders, not names
        return None
    if len(parts) == 1:
        return parts[0], ("free", parts[0])
    if len(parts) == 2 and SIMPLE.match(parts[1]) and not parts[1].startswith(("operator", "__")):
        cls, meth = parts
        if meth == cls:
            return f"{cls}_ctor", ("ctor", cls)
        return f"{cls}_{meth}", ("meth", cls, meth)
    return None


def map_body(s, fn):
    """Apply fn to a source's text outside line 1 (the salvage header) and string literals."""
    head, nl, body = s.partition("\n")
    out, pos = [], 0
    for m in STRING.finditer(body):
        out.append(fn(body[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(fn(body[pos:]))
    return head + nl + "".join(out)


def current_aliases():
    if not os.path.exists(HEADER):
        return {}
    # as written, or as clang-format (tools/format.py) lays it out: aligned, or continued on the next line
    return dict(re.findall(r"^#define (\w+)[ \t]+(?:\\[ \t]*\r?\n[ \t]*)?(\w+)[ \t]+/\*",
                           open(HEADER, encoding="utf-8").read(), re.M))


def main():
    apply = "--apply" in sys.argv
    srcs = c_sources()
    ondisk = {f: open(f, encoding="utf-8", newline="").read() for f in srcs}
    # Work from the sources as they'd read without our aliases, so reruns are idempotent: expand every
    # alias (in files that include names.h) back to its mangled name first.
    table = current_aliases()
    text = dict(ondisk)
    if table:
        ap = re.compile(r"\b(" + "|".join(map(re.escape, sorted(table, key=len, reverse=True))) + r")\b")
        for f, s in ondisk.items():
            if '#include "names.h"' in s:
                text[f] = map_body(s, lambda t: ap.sub(lambda x: table[x.group(1)], t))
    used = set()
    idents = {}                     # identifier -> files using it
    for f, s in text.items():
        body = STRING.sub('""', s.split("\n", 1)[1] if "\n" in s else "")
        used.update(MANGLED.findall(body))
        for i in set(IDENT.findall(body)):
            idents.setdefault(i, set()).add(os.path.normpath(f))
    # a function's own source may already define it under the readable name: that's not a clash
    addr = {l.split("\t")[2]: l.split("\t")[0].lower()
            for l in open(os.path.join(ROOT, "functions.tsv"), encoding="utf-8") if l.startswith("0x")}
    defined_in = {}
    for tsv in ("matched.tsv", "nearmiss.tsv"):
        for l in open(os.path.join(ROOT, tsv), encoding="utf-8"):
            p = l.rstrip("\n").split("\t")
            if len(p) > 1 and p[1].startswith("0x"):
                defined_in.setdefault(p[1].lower(), set()).add(os.path.normpath(os.path.join(ROOT, p[0])))
    sdk = set()
    for h in glob.glob(os.path.join(PSYQ_INC, "*.H")) + glob.glob(os.path.join(PSYQ_INC, "*", "*.H")):
        sdk.update(IDENT.findall(open(h, encoding="latin1").read()))

    every = universe() | used
    dm = demangle(sorted(every))
    # group every known symbol by its overload key, so an alias only goes to a unique function
    groups = {}
    for n in every:
        a = alias_of(dm.get(n, n)) if dm.get(n, n) != n else None
        if a:
            groups.setdefault(a[1], set()).add(n)
    aliases, skipped = {}, {}
    by_alias = {}
    for n in sorted(used):
        a = alias_of(dm.get(n, n)) if dm.get(n, n) != n else None
        if not a:
            continue
        alias, key = a
        why = None
        if len(groups[key]) > 1:
            why = "overloaded"
        elif idents.get(alias, set()) - defined_in.get(addr.get(n), set()):
            why = "already an identifier in a .c source"
        elif alias in sdk:
            why = "Psy-Q SDK identifier"
        if why:
            skipped[n] = why
            continue
        by_alias.setdefault(alias, []).append(n)
    for alias, ns in by_alias.items():
        if len(ns) == 1:
            aliases[ns[0]] = alias
        else:
            for n in ns:
                skipped[n] = "alias clash"

    os.makedirs(os.path.dirname(HEADER), exist_ok=True)
    with open(HEADER, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("/* Generated by tools/gen_names_h.py -- do not edit.\n"
                 "   The C sources call g++-mangled functions by the name the developers wrote; each alias\n"
                 "   expands to the mangled symbol the original code linked against. */\n"
                 "#ifndef NAMES_H\n#define NAMES_H\n\n")
        for n in sorted(aliases, key=lambda n: aliases[n].lower()):
            fh.write(f"#define {aliases[n]} {n}  /* {dm[n]} */\n")
        fh.write("\n#endif\n")
    print(f"include/names.h: {len(aliases)} aliases; {len(skipped)} mangled names kept "
          f"({', '.join(f'{v}: {sum(1 for x in skipped.values() if x == v)}' for v in sorted(set(skipped.values())))})")

    if not apply:
        return
    pat = re.compile(r"\b(" + "|".join(map(re.escape, sorted(aliases, key=len, reverse=True))) + r")\b")
    changed = 0
    for f, s in text.items():
        new = map_body(s, lambda t: pat.sub(lambda x: aliases[x.group(1)], t))
        if new != s and '#include "names.h"' not in new:
            head, _, body = new.partition("\n")
            nl = "\r\n" if head.endswith("\r") else "\n"
            new = head + "\n" + '#include "names.h"' + nl + body
        if new != ondisk[f]:
            open(f, "w", encoding="utf-8", newline="").write(new)
            changed += 1
    print(f"rewrote {changed} .c sources")


if __name__ == "__main__":
    main()
