#!/usr/bin/env python3
"""Give functions their real names everywhere at once, and turn the C++ sources' `extern "C"`
declarations of them into one C++ declaration in a shared header.

    python tools/rename_fn.py ROWS.tsv [--apply]     (run from decomp/; without --apply: report only)
    python tools/rename_fn.py --check                 FUN_ rows that some .syms.json names differently

ROWS.tsv (tab-separated, '#' lines ignored):
    addr  old  symbol  decl  header  basis
  addr    0x8003434c
  old     the identifier(s) sources use now, comma-separated: FUN_8003434c, or a mangled name
          (Mess_DrawText__FiiPCciUiib) that C++ sources declare with C linkage
  symbol  the symbol the function really has: its egcs (GNU v2) mangled name, or a C name (sprintf)
  decl    its declaration, e.g. `void CBruce::SkateOffGround()` or `int Pad_Update()`; '-' when a
          header already declares it (the SDK's, or an earlier row)
  header  where decl goes / what declares it: game/bruce.h, <libgpu.h>; '-' = none
  basis   evidence for decomp/inferred_names.tsv ('-': not an inferred name, no row)

What changes, per row:
  functions.tsv col 3 (then col 5 via demangle.py --tsv); matched/reimpl/nearmiss.tsv name column
  the defining source's line-1 header
  .c sources: `old` -> the symbol in code (gen_names_h.py --apply then turns it into the alias)
  .cpp sources: declarations of `old` (extern "C" ones included) are deleted, an `#include` of the
    header is added, calls become `Name(...)` (free) or `obj->Name(...)` (method: the first argument
    is the object); the defining .cpp gets its name (methods: reported, rewrite the body by hand)
  every .syms.json: the key `old` (or `old__<params>`, C++ linkage) -> symbol
  comments in sources and headers: FUN_x -> the readable name
  the header: the declaration (members: in the class, in address order; free functions: a list at
    the end, in address order)
  inferred_names.tsv: a row per name with a basis
data/fixed keeps its FUN_ references: data_image.py resolves names ending in _<8 hex> by address.
"""
import glob
import json
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from demangle import demangle  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
INC = os.path.join(ROOT, "include")
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
KEYWORDS = {"return", "else", "case", "default", "do", "goto", "new", "delete", "sizeof", "throw"}
TYPEWORD = re.compile(r"^(?:[\w\s\*&]|::)*$")


def rel(p):
    return os.path.relpath(p, ROOT).replace(os.sep, "/")


def read(p):
    return open(p, encoding="utf-8", newline="").read()


UNDO = None   # set by --apply: originals are copied here before the first write


def write(p, s):
    if UNDO:
        b = os.path.join(UNDO, rel(p))
        if not os.path.exists(b):
            os.makedirs(os.path.dirname(b), exist_ok=True)
            open(b, "w", encoding="utf-8", newline="").write(read(p) if os.path.exists(p) else "")
    for attempt in range(10):        # something (an indexer?) briefly locks freshly written files
        try:
            open(p, "w", encoding="utf-8", newline="").write(s)
            return
        except OSError:
            if attempt == 9:
                raise
            time.sleep(1)


def mask(s):
    """s with comments and string/char literals blanked (same length, newlines kept)."""
    return TOKEN.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)) if m.group(0)[0] == "/"
                     else m.group(0)[0] + " " * (len(m.group(0)) - 2) + m.group(0)[-1], s)


def comments_sub(s, fn):
    """Apply fn to the text of comments only."""
    return TOKEN.sub(lambda m: fn(m.group(0)) if m.group(0)[0] == "/" else m.group(0), s)


def match_paren(m, i):
    """Index just past the ')' matching the '(' at m[i]."""
    depth = 0
    for j in range(i, len(m)):
        if m[j] == "(":
            depth += 1
        elif m[j] == ")":
            depth -= 1
            if not depth:
                return j + 1
    raise ValueError("unbalanced parentheses")


def split_args(s):
    out, depth, cur = [], 0, ""
    for c in s:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == "," and not depth:
            out.append(cur)
            cur = ""
        else:
            cur += c
    out.append(cur)
    return [a.strip() for a in out] if any(a.strip() for a in out) else []


class Row:
    def __init__(self, p, dm):
        self.addr = int(p[0], 16)
        self.old = [o.strip() for o in p[1].split(",") if o.strip()]
        self.symbol = p[2]
        self.decl = p[3] if p[3] != "-" else None
        self.header = p[4] if p[4] != "-" else None
        self.basis = p[5] if len(p) > 5 and p[5] != "-" else None
        q = dm.get(self.symbol, self.symbol)
        self.readable = re.sub(r"\(.*$", "", q)      # CBruce::SkateOffGround / Pad_Update / sprintf
        parts = self.readable.split("::")
        self.cls = parts[0] if len(parts) == 2 else None
        self.name = parts[-1]
        self.cxx = q != self.symbol                  # mangled: C++ linkage

    def key_re(self):
        return re.compile(r"^(?:%s)(?:__\w*)?$" % "|".join(map(re.escape, self.old)))


def statement_start(m, i):
    j = i
    while j > 0 and m[j - 1] not in ";{}":
        j -= 1
    return j


def header_members():
    """Names of member functions in the game headers' classes (inside a method, a free function
    of the same name would be captured by the member: those calls get `::`)."""
    names = set()
    for h in glob.glob(os.path.join(INC, "game", "*.h")):
        m = mask(read(h))
        for c in re.finditer(r"\b(?:class|struct)\s+\w+[^;{(]*\{", m):
            depth, i = 1, c.end()
            while depth and i < len(m):
                depth += {"{": 1, "}": -1}.get(m[i], 0)
                i += 1
            names.update(re.findall(r"\b([A-Za-z_]\w*)\s*\(", m[c.end():i]))
    return names


def edit_cpp(path, s, rows, members, report):
    """Rewrite one .cpp source's uses of the rows' old names. Returns the new text."""
    m = mask(s)
    edits = []      # (start, end, replacement)
    need = set()
    for r in rows:
        pat = re.compile(r"\b(%s)\b" % "|".join(map(re.escape, r.old)))
        for x in pat.finditer(m):
            st, en = x.span()
            k = en
            while k < len(m) and m[k] in " \t\r\n":
                k += 1
            start = statement_start(m, st)
            pp = list(re.finditer(r"(?m)^[ \t]*#[^\n]*\n", m[start:st]))
            if pp:
                start += pp[-1].end()        # the statement begins after any preprocessor lines
            prefix = re.sub(r"(?m)^[ \t]*#[^\n]*$", "", m[start:st]).strip()   # not preprocessor lines
            prefix_noext = re.sub(r'^extern\s+"\s*"\s*', "", prefix)   # extern "C" (masked)
            is_declish = (prefix_noext and TYPEWORD.match(prefix_noext)
                          and not (set(re.findall(r"\w+", prefix_noext)) & KEYWORDS)
                          and not re.search(r"(?<!:):(?!:)", prefix_noext))
            if k < len(m) and m[k] == "(":
                close = match_paren(m, k)
                t = close
                while t < len(m) and m[t] in " \t\r\n":
                    t += 1
                if is_declish and t < len(m) and m[t] == ";":
                    # a declaration: delete the statement (and its line if nothing else is on it)
                    a, b = start, t + 1
                    while a < st and m[a].isspace():  # (comments are blank in m)
                        a += 1                       # the statement proper starts here
                    line_start = s.rfind("\n", 0, a) + 1
                    line_end = s.find("\n", b)
                    line_end = len(s) if line_end < 0 else line_end
                    if (not s[line_start:a].strip()
                            and re.fullmatch(r"\s*(/\*.*?\*/|//.*)?\s*", s[b:line_end])):
                        a, b = line_start, min(line_end + 1, len(s))   # the whole line
                    edits.append((a, b, ""))
                    need.add(r)
                    continue
                if is_declish and t < len(m) and m[t] == "{":
                    if r.cls:
                        report.append(f"{rel(path)}: defines {r.old[0]} -> rewrite as {r.readable} by hand")
                        edits.append((st, en, r.symbol))   # keep it linking meanwhile
                    else:
                        edits.append((st, en, r.name))
                    continue
                if is_declish:
                    report.append(f"{rel(path)}: unrecognised use of {x.group(1)} at offset {st}")
                    continue
                # a call
                if r.cls:
                    args = split_args(s[k + 1:close - 1])
                    if not args:
                        report.append(f"{rel(path)}: method call without an object: {s[st:close]}")
                        continue
                    a0 = args[0]
                    if a0 == "this":
                        obj = ""
                    elif re.fullmatch(r"&\s*[\w.\[\]]+(?:->[\w.\[\]]+)*", a0):
                        obj = a0[1:].strip() + "."
                    elif re.fullmatch(r"[\w.\[\]]+(?:->[\w.\[\]]+)*", a0):
                        obj = a0 + "->"
                    else:
                        obj = f"({a0})->"
                    # replace `OLD(a0, ` only, so calls nested in the other arguments get their own edits
                    depth, j = 0, k + 1
                    while j < close - 1:
                        c = m[j]
                        if c in "([{":
                            depth += 1
                        elif c in ")]}":
                            depth -= 1
                        elif c == "," and not depth:
                            break
                        j += 1
                    if j < close - 1:
                        j += 1
                        while m[j].isspace():
                            j += 1
                    edits.append((st, j, f"{obj}{r.name}("))
                else:
                    name = ("::" + r.name) if r.name in members else r.name
                    edits.append((st, en, name))
                need.add(r)
            else:
                # not a call: a function pointer / callback
                if r.cls:
                    report.append(f"{rel(path)}: method {x.group(1)} used as a value; fix by hand")
                    edits.append((st, en, r.symbol))
                else:
                    edits.append((st, en, r.name))
                need.add(r)
    if not edits:
        return s, set()
    edits.sort()
    for (a1, b1, _x), (a2, b2, _y) in zip(edits, edits[1:]):
        if a2 < b1:
            raise ValueError(f"{rel(path)}: overlapping edits {(a1, b1, s[a1:b1])} and {(a2, b2, s[a2:b2])}")
    out, pos = [], 0
    for a, b, t in edits:
        out.append(s[pos:a])
        out.append(t)
        pos = b
    out.append(s[pos:])
    s = "".join(out)
    # an extern "C" block left empty
    s = re.sub(r'\n?extern "C" \{\s*\}[ \t]*\r?\n', "\n", s)
    return s, {r.header for r in need if r.header}


def add_include(s, h):
    inc = f'#include <{h[1:-1]}>' if h.startswith("<") else f'#include "{h}"'
    if re.search(r"^\s*" + re.escape(inc) + r"\s*$", s, re.M):
        return s
    nl = "\r\n" if "\r\n" in s else "\n"
    if h.startswith("<"):
        # into the SDK block: after its last #include
        ms = list(re.finditer(r'extern "C" \{(?:[^{}]*?)\n\}', s))
        for blk in ms:
            if "#include <" in blk.group(0):
                body = blk.group(0)
                last = max(i.end() for i in re.finditer(r"^\s*#include <[^>]+>[^\r\n]*", body, re.M))
                indent = "\t"
                body = body[:last] + nl + indent + inc + body[last:]
                return s[:blk.start()] + body + s[blk.end():]
        # a new block: the SDK headers need <sys/types.h> first, and libgpu.h needs libgte.h
        pre = ["#include <sys/types.h>"] + (["#include <libgte.h>"] if h == "<libgpu.h>" else [])
        lines = "".join(f"\t{x}{nl}" for x in pre + [inc])
        return insert_after_header(s, f'extern "C" {{{nl}{lines}}}{nl}')
    def top_level(pos):          # not inside #if/#ifdef ... #endif
        d = 0
        for x in re.finditer(r"(?m)^[ \t]*#[ \t]*(if|endif)", s[:pos]):
            d += 1 if x.group(1) == "if" else -1
        return d == 0
    ms = [x for x in re.finditer(r'^#include "game/[^"]+"[^\r\n]*', s, re.M) if top_level(x.start())]
    if ms:
        # keep game/ includes sorted
        names = [x.group(0) for x in ms]
        if names == sorted(names) and len(ms) == len({x.start() for x in ms}):
            for x in ms:
                if x.group(0) > inc:
                    return s[:x.start()] + inc + nl + s[x.start():]
        last = ms[-1]
        return s[:last.end()] + nl + inc + s[last.end():]
    ms = list(re.finditer(r'extern "C" \{(?:[^{}]*?)\n\}', s))
    if ms and "#include" in ms[0].group(0):
        e = ms[0].end()
        return s[:e] + nl + nl + inc + s[e:]
    return insert_after_header(s, inc + nl)


def insert_after_header(s, text):
    """After the leading comments (salvage header + description)."""
    m = re.match(r"(?:\s*/\*.*?\*/[ \t]*\r?\n|\s*//[^\n]*\n)*", s, re.S)
    e = m.end() if m else 0
    nl = "\r\n" if "\r\n" in s else "\n"
    return s[:e] + text + nl + s[e:] if e else text + nl + s


def member_decl(r):
    """`void CBruce::SkateOffGround()` -> `void SkateOffGround();`"""
    d = r.decl.replace(f"{r.cls}::", "", 1)
    return d.rstrip(";") + ";"


def add_to_header(r, report):
    if not r.header or r.header.startswith("<") or not r.decl:
        return
    path = os.path.join(INC, r.header)
    if not os.path.exists(path):
        guard = "GAME_" + re.sub(r"\W", "_", os.path.basename(r.header)).upper()
        write(path, f"#ifndef {guard}\n#define {guard}\n\n#include \"game/types.h\"\n\n#endif\n")
        report.append(f"created include/{r.header}: write its description comment")
    s = read(path)
    tag = f"/* 0x{r.addr:08x} */"
    if re.search(r"\b%s\s*\(.*0x%08x" % (re.escape(r.name), r.addr), s):
        return  # already declared
    if r.cls:
        cm = re.search(r"^(?:class|struct)\s+%s\b[^;{]*\{" % re.escape(r.cls), s, re.M)
        if not cm:
            report.append(f"{r.header}: no class {r.cls} for {r.readable}; add it by hand")
            return
        end = s.find("\n};", cm.end())
        body = s[cm.end():end]
        line = f"\t{member_decl(r)} {tag}"
        best = None
        for x in re.finditer(r"^.*\(.*\).*?/\*.*?0x([0-9a-f]{8}).*$", body, re.M):
            if int(x.group(1), 16) > r.addr:
                best = cm.end() + x.start()
                break
        if best is None:
            best = end + 1
        s = s[:best] + line + "\n" + s[best:]
    else:
        line = f"{r.decl.rstrip(';')}; {tag}"
        marker = "/* functions */\n"
        if marker not in s:
            e = s.rfind("#endif")
            s = s[:e].rstrip("\n") + "\n\n" + marker + "\n" + s[e:]
        start = s.index(marker) + len(marker)
        e = s.rfind("#endif")
        block = s[start:e]
        lines = [l for l in block.split("\n") if l.strip()]
        lines.append(line)
        key = lambda l: int(re.search(r"0x([0-9a-f]{8}) \*/\s*$", l).group(1), 16) if re.search(r"0x([0-9a-f]{8}) \*/\s*$", l) else 0  # noqa: E731
        lines.sort(key=key)
        s = s[:start] + "\n".join(lines) + "\n\n" + s[e:]
    write(path, s)


def update_tables(rows, report):
    by = {r.addr: r for r in rows}
    p = os.path.join(ROOT, "functions.tsv")
    lines = read(p).split("\n")
    for i, l in enumerate(lines):
        if l.startswith("0x"):
            q = l.split("\t")
            r = by.get(int(q[0], 16))
            if r and q[2] != r.symbol:
                q[2] = r.symbol
                lines[i] = "\t".join(q)
    write(p, "\n".join(lines))
    for t in ("matched.tsv", "reimpl.tsv", "nearmiss.tsv"):
        p = os.path.join(ROOT, t)
        lines = read(p).split("\n")
        for i, l in enumerate(lines):
            q = l.split("\t")
            if len(q) > 4 and q[1].startswith("0x"):
                r = by.get(int(q[1], 16))
                if r and q[4] in r.old:
                    q[4] = r.symbol
                    lines[i] = "\t".join(q)
        write(p, "\n".join(lines))
    p = os.path.join(ROOT, "inferred_names.tsv")
    s = read(p)
    have = {l.split("\t")[0] for l in s.split("\n") if l.startswith("0x")}
    add = [f"0x{r.addr:08x}\t{r.symbol}\t{r.basis}" for r in rows if r.basis and f"0x{r.addr:08x}" not in have]
    if add:
        write(p, s.rstrip("\n") + "\n" + "\n".join(add) + "\n")


NAMES_H = os.path.join(INC, "names.h")
DEFINE = re.compile(r"^#define (\w+)[ \t]+(?:\\[ \t]*\r?\n[ \t]*)?(\w+)[ \t]+/\*[ \t]*(.*?)[ \t]*\*/", re.M)


def c_aliases(rows, srcs, report):
    """The readable names .c sources get for the rows' C++ symbols (include/names.h aliases, as
    tools/gen_names_h.py makes them: Name, or Class_Method). An alias is skipped (the source gets
    the mangled name) when names.h has it for another symbol, another function in functions.tsv
    would have it too (an overload), or it's already an identifier in some .c source or the SDK."""
    existing = {a: (s, c) for a, s, c in DEFINE.findall(read(NAMES_H))}
    taken = {}
    for l in read(os.path.join(ROOT, "functions.tsv")).split("\n"):
        if l.startswith("0x"):
            q = l.split("\t")
            m = re.match(r"^(?:(\w+)::)?(\w+)\(", q[4]) if len(q) > 4 else None
            if m:
                taken.setdefault(f"{m.group(1)}_{m.group(2)}" if m.group(1) else m.group(2), set()).add(int(q[0], 16))
    idents = set()
    for f in srcs:
        if f.endswith(".c"):
            idents.update(re.findall(r"\b[A-Za-z_]\w*\b", mask(read(f))))
    names, new = {}, {}
    for r in rows:
        if not r.cxx or r.name == r.cls or r.name.startswith(("operator", "_")):
            continue
        alias = f"{r.cls}_{r.name}" if r.cls else r.name
        if alias in existing:
            if existing[alias][0] == r.symbol:
                names[r.symbol] = alias
            continue
        others = taken.get(alias, set()) - {r.addr}
        if others or alias in idents or alias in new:
            report.append(f"names.h: no alias {alias} for {r.symbol}"
                          f" ({'overload' if others else 'clash'}); .c sources use the mangled name")
            continue
        names[r.symbol] = alias
        new[alias] = (r.symbol, f"{r.readable}({demangle([r.symbol])[r.symbol].split('(', 1)[1]}")
    return names, new


def write_names_h(new, rows):
    """Add aliases to include/names.h, and point existing ones at renamed symbols, in
    gen_names_h.py's order and layout (format.py realigns)."""
    s = read(NAMES_H)
    entries = {a: (sym, c) for a, sym, c in DEFINE.findall(s)}
    renamed = {o: r for r in rows for o in r.old}
    moved = {a: renamed[sym] for a, (sym, c) in entries.items() if sym in renamed}
    if not new and not moved:
        return
    for a, r in moved.items():
        entries[a] = (r.symbol, demangle([r.symbol])[r.symbol])
    entries.update(new)
    head = s[:s.index("#define", s.index("#define NAMES_H") + 1)]
    body = "".join(f"#define {a} {sym}  /* {c} */\n" for a, (sym, c) in sorted(entries.items(), key=lambda e: e[0].lower()))
    write(NAMES_H, head + body + "\n#endif\n")


def run(rows, apply):
    report = []
    members = header_members()
    srcs = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True)
                  + glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    heads = sorted(glob.glob(os.path.join(INC, "game", "*.h")))
    readable = {}
    for r in rows:
        for o in r.old:
            if o.startswith("FUN_"):
                readable[o] = r.readable
    fun_re = re.compile(r"\b(%s)\b" % "|".join(map(re.escape, readable))) if readable else None
    any_old = re.compile(r"\b(%s)\b" % "|".join(re.escape(o) for r in rows for o in r.old))
    c_names, new_aliases = c_aliases(rows, srcs, report)
    changed, pending = [], []
    for f in srcs:
        s = read(f)
        # syms.json
        sp = os.path.splitext(f)[0] + ".syms.json"
        if os.path.exists(sp):
            d = json.load(open(sp))
            nd = {}
            for k, v in d.items():
                for r in rows:
                    if r.key_re().match(k) and int(v, 16) == r.addr:
                        k = r.symbol
                        break
                if k in nd and nd[k] != v:
                    report.append(f"{rel(sp)}: {k} maps to {nd[k]} and {v}")
                nd.setdefault(k, v)
            if nd != d:
                changed.append(rel(sp))
                pending.append((sp, json.dumps(nd, separators=(",", ":"))))
        if not any_old.search(s):
            continue
        new = s
        head, nl, body = new.partition("\n")
        for r in rows:
            for o in r.old:
                head = re.sub(r"\b%s\b" % re.escape(o), r.symbol, head)
        new = head + nl + body
        if f.endswith(".c"):
            mm = mask(new)
            hits = [x for r in rows for x in re.finditer(r"\b(%s)\b" % "|".join(map(re.escape, r.old)), mm)
                    if x.start() > len(head)]
            sub = {o: c_names.get(r.symbol, r.symbol) for r in rows for o in r.old}
            if any(c_names.get(r.symbol) for r in rows for x in hits if x.group(1) in r.old) \
                    and '#include "names.h"' not in new:
                h, nl0, b = new.partition("\n")
                new = h + nl0 + '#include "names.h"' + ("\r\n" if h.endswith("\r") else "\n") + b
                mm = mask(new)
                hits = [x for r in rows for x in re.finditer(r"\b(%s)\b" % "|".join(map(re.escape, r.old)), mm)
                        if x.start() > len(head)]
            out, pos = [], 0
            for x in sorted(hits, key=lambda x: x.start()):
                out.append(new[pos:x.start()])
                out.append(sub[x.group(1)])
                pos = x.end()
            out.append(new[pos:])
            new = "".join(out)
        else:
            touched = [r for r in rows if re.search(r"\b(%s)\b" % "|".join(map(re.escape, r.old)), mask(body))]
            if touched:
                h2, nl2, b2 = new.partition("\n")
                b2, heads_needed = edit_cpp(f, "\n" + b2, touched, members, report)
                new = h2 + nl2 + b2[1:]
                for h in sorted(heads_needed):
                    new = add_include(new, h)
        if fun_re:
            new = comments_sub(new, lambda c: fun_re.sub(lambda x: readable[x.group(1)], c))
        if new != s:
            changed.append(rel(f))
            pending.append((f, new))
    for h in heads:
        s = read(h)
        new = comments_sub(s, lambda c: fun_re.sub(lambda x: readable[x.group(1)], c)) if fun_re else s
        if new != s:
            changed.append(rel(h))
            pending.append((h, new))
    if apply:
        for f, t in pending:
            write(f, t)
        for r in rows:
            add_to_header(r, report)
        update_tables(rows, report)
        write_names_h(new_aliases, rows)
    print(f"{'changed' if apply else 'would change'} {len(changed)} files")
    for l in report:
        print("  " + l)
    return changed


def check():
    fn = {}
    for l in read(os.path.join(ROOT, "functions.tsv")).split("\n"):
        if l.startswith("0x"):
            q = l.split("\t")
            fn[int(q[0], 16)] = q[2]
    bad = 0
    for sp in glob.glob(os.path.join(ROOT, "src", "**", "*.syms.json"), recursive=True):
        for k, v in json.load(open(sp)).items():
            a = int(v, 16)
            if a in fn and fn[a].startswith("FUN_") and not k.startswith("FUN_") and k != "gp":
                print(f"{rel(sp)}: {k} = {v}, functions.tsv says {fn[a]}")
                bad += 1
    print(f"{bad} half-renamed references")


def main():
    if sys.argv[1:] == ["--check"]:
        return check()
    rows_path = sys.argv[1]
    apply = "--apply" in sys.argv
    if apply:
        global UNDO
        import time
        UNDO = os.path.join(ROOT, "..", "build", "rename_fn_undo", time.strftime("%Y%m%d-%H%M%S"))
        print("originals saved to", os.path.normpath(UNDO))
    lines = [l.rstrip("\r\n").split("\t") for l in open(rows_path, encoding="utf-8")
             if l.strip() and not l.startswith("#")]
    dm = demangle([p[2] for p in lines])
    rows = [Row(p, dm) for p in lines]
    for r in rows:
        if r.cxx and r.decl and r.name not in r.decl:
            sys.exit(f"0x{r.addr:08x}: decl {r.decl!r} doesn't name {r.readable}")
    run(rows, apply)


if __name__ == "__main__":
    main()
