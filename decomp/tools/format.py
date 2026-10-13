"""Format matching sources: clang-format (decomp/.clang-format), then a blank-line layout pass.

    python tools/format.py [--no-clang] [file|dir]...      (default: src)

The layout pass only inserts blank lines and indents preprocessor lines inside `extern "C"`
blocks, so codegen is unaffected; it checks that per file before writing.

Top level (file scope and `extern "C"` blocks): declarations are grouped by kind (preprocessor,
typedefs, externs, variable definitions, prototypes) with a blank line between groups, and every
multi-line construct (struct, function, extern block) is set off by blank lines. Comments stay
attached to the declaration that follows them.

Function bodies: a blank line around every control statement (if/for/while/do/switch, braced or
not, an if/else chain counting as one), after a group of declarations, and between a call and an
assignment that follows it. Nothing is inserted at the start or end of a block, or next to
labels and preprocessor lines.
"""
import os
import re
import shutil
import subprocess
import sys
import sysconfig
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent  # decomp/

KEYWORDS = {"return", "goto", "break", "continue", "delete", "throw", "case", "default", "else",
            "if", "for", "while", "do", "switch", "sizeof", "new", "typedef", "extern", "using"}
CONTROL = re.compile(r"(if|for|while|switch)\s*\(|do\b|else\b|\{")
JUMP = re.compile(r"(return|break|continue|goto)\b")
LABEL = re.compile(r"(case\b.*|default\s*|[A-Za-z_]\w*\s*|(public|private|protected)\s*):(?!:)")
DECL = re.compile(r"((const|volatile|static|register|unsigned|signed|struct|union|enum|class)\s+)*"
                  r"[A-Za-z_][\w:<>]*[\s*&]+(const\s+)?\**[A-Za-z_]\w*\s*(\[[^]]*\])*"
                  r"\s*(\(.*\)|=.*)?(\s*,\s*[*&]*[A-Za-z_]\w*\s*(\[[^]]*\])*\s*(=.*)?)*;$")
ASSIGN = re.compile(r"[^=!<>]=[^=]|(\+\+|--)\s*;$|^(\+\+|--)")
PP_COND = re.compile(r"#\s*(if|ifdef|ifndef|elif|else|endif)\b")


# ---------------------------------------------------------------- lexing

def strip_code(line, in_comment):
    """(line with comments and string/char literals removed, in_comment after the line)."""
    out, i, n = [], 0, len(line)
    while i < n:
        if in_comment:
            j = line.find("*/", i)
            if j < 0:
                return "".join(out), True
            i, in_comment = j + 2, False
            out.append(" ")
        elif line.startswith("/*", i):
            i, in_comment = i + 2, True
        elif line.startswith("//", i):
            break
        elif line[i] in "\"'":
            q, i = line[i], i + 1
            while i < n and line[i] != q:
                i += 2 if line[i] == "\\" else 1
            i += 1
            out.append(q + q)
        else:
            out.append(line[i])
            i += 1
    return "".join(out), in_comment


class Src:
    def __init__(self, lines):
        self.lines = lines
        self.code, self.pp = [], []
        in_comment, in_pp = False, False
        for l in lines:
            c, in_comment = strip_code(l, in_comment)
            is_pp = in_pp or c.lstrip().startswith("#")
            in_pp = is_pp and l.rstrip().endswith("\\")
            self.pp.append(is_pp)
            self.code.append("" if is_pp else c.strip())


# ---------------------------------------------------------------- segmentation

class Item:
    def __init__(self, start, end, kind):
        self.start, self.end, self.kind = start, end, kind  # lines [start, end)
        self.blocks = []  # inner (start, end) ranges, with the context to format them in


def paren_close(s, i):
    depth = 0
    for j in range(i, len(s)):
        depth += {"(": 1, ")": -1}.get(s[j], 0)
        if depth == 0:
            return j
    return -1


def classify_stmt(text):
    first = re.match(r"[A-Za-z_]\w*", text)
    if CONTROL.match(text):
        return "control"
    if LABEL.fullmatch(text) or LABEL.match(text) and text.endswith("{"):
        return "label"
    if JUMP.match(text):
        return "jump"
    if first and first.group() not in KEYWORDS and DECL.fullmatch(text):
        return "decl"
    m = re.match(r"[A-Za-z_][\w.\[\]:>\-]*\s*\(", text)
    if m:
        close = paren_close(text, m.end() - 1)
        if close >= 0 and text[close + 1:].strip() == ";":
            return "call"
    if ASSIGN.search(text):
        return "assign"
    return "other"


def classify_top(text, multiline_block):
    if text.startswith(("extern \"C\"", "extern \"\"")) and "{" in text:
        return "block"
    if multiline_block:
        return "block"
    if text.startswith("typedef") or re.match(r"(struct|class|union|enum)\b", text):
        return "type"
    head = text.split("=", 1)[0]
    if "(" in head and text.endswith(");"):
        return "proto"
    if text.startswith("extern"):
        return "extern"
    return "var"


def segment(src, lo, hi, ctx):
    """Split lines [lo, hi) into items (one statement / declaration / comment / blank each)."""
    items, i = [], lo
    while i < hi:
        line = src.lines[i].strip()
        if not line:
            items.append(Item(i, i + 1, "blank"))
            i += 1
            continue
        if src.pp[i]:
            j = i + 1
            while j < hi and src.pp[j] and src.lines[j - 1].rstrip().endswith("\\"):
                j += 1
            cond = PP_COND.match(line)
            items.append(Item(i, j, "ppcond" if cond else "pp"))
            i = j
            continue
        if not src.code[i]:
            items.append(Item(i, i + 1, "comment"))
            i += 1
            continue
        # Accumulate a statement: until braces and parens balance and it ends in ';' or '}'.
        j, braces, parens, text, multiline_block = i, 0, 0, [], False
        while j < hi:
            c = src.code[j]
            if c:
                text.append(c)
            for ch in c:
                if ch == "{":
                    braces += 1
                elif ch == "}":
                    braces -= 1
                elif ch == "(":
                    parens += 1
                elif ch == ")":
                    parens -= 1
            if braces > 0 and j > i:
                multiline_block = True
            j += 1
            if braces <= 0 and parens <= 0 and c and (c.endswith(";") or c.endswith("}")):
                # an if/else chain continues past a braceless body
                if j < hi and re.match(r"else\b", src.code[j]):
                    continue
                break
            if braces <= 0 and parens <= 0 and c and LABEL.fullmatch(c) and j == i + 1:
                break
        joined = " ".join(text)
        kind = classify_top(joined, multiline_block or (braces == 0 and j - i > 1 and "{" in joined)) \
            if ctx in ("top", "extern") else classify_stmt(joined)
        item = Item(i, j, kind)
        item.blocks = inner_blocks(src, i, j, ctx)
        items.append(item)
        i = j
    return items


def inner_blocks(src, lo, hi, ctx):
    """Brace-delimited line ranges directly inside [lo, hi), with the context each is in."""
    blocks, depth, head, start = [], 0, [], None
    for k in range(lo, hi):
        c = src.code[k]
        for p, ch in enumerate(c):
            if ch == "{":
                depth += 1
                if depth == 1:
                    start = k + 1 if not c[p + 1:].strip() else None
                    h = " ".join(head) + " " + c[:p]
                    head = []
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    if start is not None and not c[:p].strip():
                        blocks.append((start, k, block_ctx(h.strip(), ctx)))
                    start = None
            elif depth == 0:
                pass
        if depth == 0:
            tail = re.split(r"[;}]", c)[-1]
            head.append(tail)
    return blocks


def block_ctx(head, outer):
    if head.startswith(("extern \"C\"", "extern \"\"")):
        return "extern"
    if re.search(r"(^|\s)(struct|class|union)\b", head) and "(" not in head:
        return "member"
    if re.search(r"(^|\s)enum\b", head) or head.rstrip().endswith("="):
        return "opaque"
    if outer in ("top", "extern", "member") and "(" not in head:
        return "opaque"  # aggregate initializer etc.
    return "body"


# ---------------------------------------------------------------- layout rules

def wants_blank(a, b, ctx):
    """Blank line between consecutive (non-comment) items of kinds a and b?"""
    if ctx in ("top", "extern"):
        if "ppcond" in (a, b):
            return False
        return a == "block" or b == "block" or a != b
    if ctx != "body" or {"pp", "ppcond", "label"} & {a, b}:
        return False
    if a == "control" or b == "control":
        return True
    if a == "decl" and b != "decl":
        return True
    return a == "call" and b == "assign"


def layout(src, lo, hi, ctx):
    out = []
    if ctx == "opaque":
        return src.lines[lo:hi]
    items = segment(src, lo, hi, ctx)
    prev = None  # kind of the previous non-comment item since the last blank line
    pending = []  # comment items waiting for the item they belong to
    for it in items:
        if it.kind == "blank":
            out += [src.lines[k] for c in pending for k in range(c.start, c.end)]
            pending = []
            out.append(src.lines[it.start])
            prev = None
            continue
        if it.kind == "comment":
            pending.append(it)
            continue
        if prev is not None and wants_blank(prev, it.kind, ctx):
            out.append("")
        for c in pending:
            out += src.lines[c.start:c.end]
        pending = []
        k = it.start
        for (s, e, sub) in it.blocks:
            out += src.lines[k:s]
            out += layout(src, s, e, sub)
            k = e
        body = src.lines[k:it.end]
        if ctx == "extern" and it.kind in ("pp", "ppcond"):
            body = ["\t" + body[0].lstrip()] + body[1:]
        out += body
        prev = it.kind
    for c in pending:
        out += src.lines[c.start:c.end]
    return out


def normalise(lines):
    return [l.strip() for l in lines if l.strip()]


def format_layout(path):
    raw = path.read_bytes().decode("latin-1")
    nl = "\r\n" if "\r\n" in raw else "\n"
    lines = raw.split(nl)
    src = Src(lines)
    new = layout(src, 0, len(lines), "top")
    if normalise(new) != normalise(lines):
        raise SystemExit(f"{path}: layout pass would change more than whitespace; not written")
    if new != lines:
        # a file clang-format just wrote can still be held open briefly (virus scanners, editors'
        # file watchers), which fails the open with EINVAL/EACCES on Windows
        for tries in range(20):
            try:
                path.write_bytes(nl.join(new).encode("latin-1"))
                break
            except OSError:
                if tries == 19:
                    raise
                time.sleep(0.1)


# ---------------------------------------------------------------- driver

def clang_format_exe():
    exe = shutil.which("clang-format")
    if exe:
        return exe
    for scheme in (f"{os.name}_user", None):
        d = sysconfig.get_path("scripts", scheme) if scheme else sysconfig.get_path("scripts")
        for name in ("clang-format.exe", "clang-format"):
            p = Path(d) / name
            if p.exists():
                return str(p)
    raise SystemExit("clang-format not found (pip install --user clang-format)")


def main(argv):
    clang = "--no-clang" not in argv
    paths = [a for a in argv if a != "--no-clang"] or [str(HERE / "src")]
    files = []
    for p in map(Path, paths):
        files += sorted(f for f in p.rglob("*") if f.suffix in (".c", ".cpp", ".h")) \
            if p.is_dir() else [p]
    before = {f: f.read_bytes() for f in files}
    if clang:
        exe = clang_format_exe()
        for k in range(0, len(files), 50):
            subprocess.run([exe, "-i", "--style=file"] + [str(f) for f in files[k:k + 50]],
                           check=True, cwd=HERE)
    for f in files:
        format_layout(f)
    changed = sum(f.read_bytes() != before[f] for f in files)
    print(f"{changed}/{len(files)} files changed")


if __name__ == "__main__":
    main(sys.argv[1:])
