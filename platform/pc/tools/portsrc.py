"""Source transforms that turn a matching-decomp TU into one the native i686 compiler can build.

The decompiled sources are written for egcs 1.1.2 on MIPS. Two things in them don't carry over:

* Inline MIPS asm. Every `__asm__` in the tree drives the GTE (mtc2/ctc2/lwc2/swc2/mfc2/cfc2) or is
  dead code kept for matching (HandleJump's lone `mult`). The port has no GTE: the GTE asm is in
  the #else branches of PC_PORT ones on the geometry maths (pc_geom.h). `translate_asm` rewrites
  each statement into C (before the preprocessor, so the dead branches too); a GTE instruction
  becomes a call that stops the build if the code is compiled (PC_GteAsmInPcBuild, pc_game.h).
* C++ virtual functions. egcs puts a class's vtable pointer after its data members and lays out
  vtable entries as {short delta, short index, pfn}; modern g++ uses the Itanium ABI (vptr at
  offset 0), which would shift every field. `transform_cxx` strips `virtual`, inserts an explicit
  `__vptr` member where egcs puts it, stores the class's egcs-layout vtable (game/data) in
  constructors and destructors, and records each class's virtual slots so the build can route
  virtual calls through those vtables (see build.py, "virtual dispatch").
"""
import re

# --------------------------------------------------------------------------------------------
# lexical helpers


def _skip_ws_comments(s, i):
    n = len(s)
    while i < n:
        if s[i].isspace():
            i += 1
        elif s.startswith("/*", i):
            j = s.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif s.startswith("//", i):
            j = s.find("\n", i)
            i = n if j < 0 else j + 1
        else:
            break
    return i


def _match_paren(s, i, open_ch="(", close_ch=")"):
    """s[i] == open_ch; return index just past the matching close, skipping strings/comments."""
    depth = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c == '"' or c == "'":
            q = c
            i += 1
            while i < n and s[i] != q:
                i += 2 if s[i] == "\\" else 1
        elif s.startswith("/*", i):
            j = s.find("*/", i + 2)
            i = n - 1 if j < 0 else j + 1
        elif s.startswith("//", i):
            j = s.find("\n", i)
            i = n - 1 if j < 0 else j
        elif c == open_ch:
            depth += 1
        elif c == close_ch:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise ValueError("unbalanced %s" % open_ch)


def _split_top(s, sep):
    """Split on `sep` at paren depth 0, outside strings."""
    out, depth, cur, i = [], 0, [], 0
    while i < len(s):
        c = s[i]
        if c in "\"'":
            q = c
            j = i + 1
            while j < len(s) and s[j] != q:
                j += 2 if s[j] == "\\" else 1
            cur.append(s[i:j + 1])
            i = j + 1
            continue
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        if c == sep and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(c)
        i += 1
    out.append("".join(cur))
    return out


# --------------------------------------------------------------------------------------------
# inline asm -> C

_ASM_RE = re.compile(r"\b(__asm__|asm)\s*(volatile|__volatile__)?\s*\(")


def _template_pieces(tmpl):
    """Parse an asm template made of string literals and `#param` stringifications into a list of
    ('s', text) / ('p', name) pieces."""
    pieces = []
    i = 0
    while i < len(tmpl):
        i = _skip_ws_comments(tmpl, i)
        if i >= len(tmpl):
            break
        if tmpl[i] == '"':
            j = i + 1
            buf = []
            while tmpl[j] != '"':
                if tmpl[j] == "\\":
                    buf.append({"n": "\n", "t": "\t"}.get(tmpl[j + 1], tmpl[j + 1]))
                    j += 2
                else:
                    buf.append(tmpl[j])
                    j += 1
            pieces.append(("s", "".join(buf)))
            i = j + 1
        elif tmpl[i] == "#":
            m = re.match(r"#\s*(\w+)", tmpl[i:])
            pieces.append(("p", m.group(1)))
            i += m.end()
        else:
            raise ValueError("unexpected asm template text: %r" % tmpl[i:i + 20])
    return pieces


def _operand_exprs(part):
    """'"r"(a), "=r"(b)' -> [('r','a'), ('=r','b')]"""
    out = []
    for op in _split_top(part, ","):
        op = op.strip()
        if not op:
            continue
        m = re.match(r'(?:\[\w+\]\s*)?"([^"]*)"\s*\((.*)\)\s*$', op, re.S)
        if not m:
            raise ValueError("bad asm operand %r" % op)
        out.append((m.group(1), m.group(2).strip()))
    return out


def _reg_expr(tok, params):
    """'$5' -> '5'; '$' followed by a macro parameter piece -> '(param)'."""
    tok = tok.strip()
    if tok.startswith("$"):
        tok = tok[1:]
    if tok in params:
        return "(%s)" % tok
    return str(int(tok))


_LOADS = {"lw": "int", "lh": "short", "lhu": "unsigned short", "lb": "signed char",
          "lbu": "unsigned char"}
_STORES = {"sw": "int", "sh": "short", "sb": "char"}


def translate_asm_stmt(tmpl, outputs, inputs):
    """Return C code for one asm statement (without trailing ';'). MIPS general registers used as
    temporaries inside the template become locals of the emitted block."""
    pieces = _template_pieces(tmpl)
    params = {p for k, p in pieces if k == "p"}
    text = "".join(p for k, p in pieces)
    ops = [e for _, e in outputs] + [e for _, e in inputs]
    used = set()
    temps = set()

    def gpr(tok):
        """A general-register operand (source or destination) as a C lvalue/rvalue."""
        tok = tok.strip()
        m = re.fullmatch(r"%(\d+)", tok)
        if m:
            used.add(int(m.group(1)))
            return "(%s)" % ops[int(m.group(1))]
        if tok in ("$0", "$zero"):
            return "0"
        m = re.fullmatch(r"\$(\d+)", tok)
        if m:
            temps.add(int(m.group(1)))
            return "__r%s" % m.group(1)
        raise ValueError("unsupported asm operand %r" % tok)

    def mem(tok):
        m = re.fullmatch(r"(-?\w*)\s*\(\s*(\S+?)\s*\)", tok.strip())
        return "((char *)%s + %s)" % (gpr(m.group(2)), m.group(1) or "0")

    stmts = []
    for ins in re.split(r"[;\n]", text):
        ins = ins.strip()
        if not ins:
            continue
        m = re.match(r"([\w.]+)\s*(.*)$", ins)
        mnem = m.group(1)
        args = _split_top(m.group(2), ",") if m.group(2) else []
        args = [a.strip() for a in args]
        if mnem == "nop":
            continue
        if mnem in ("mtc2", "ctc2", "mfc2", "cfc2", "lwc2", "swc2", "cop2", ".word"):
            # The port has no GTE: the matching sources' GTE asm is in their #else branches (the
            # PC_PORT ones use pc_geom.h), which the preprocessor drops. One in code the PC build
            # compiles stops it (pc_game.h).
            stmts.append("PC_GteAsmInPcBuild()")
        elif mnem in _LOADS:
            stmts.append("%s = *(%s *)%s" % (gpr(args[0]), _LOADS[mnem], mem(args[1])))
        elif mnem in _STORES:
            stmts.append("*(%s *)%s = (%s)%s" % (_STORES[mnem], mem(args[1]), _STORES[mnem],
                                                 gpr(args[0])))
        elif mnem == "lui":
            stmts.append("%s = (unsigned int)(%s) << 16" % (gpr(args[0]), args[1]))
        elif mnem in ("ori", "addiu", "addu", "or", "and", "andi", "subu"):
            op = {"ori": "|", "or": "|", "and": "&", "andi": "&", "addiu": "+", "addu": "+",
                  "subu": "-"}[mnem]
            b = args[2] if not args[2].startswith(("$", "%")) else gpr(args[2])
            stmts.append("%s = (unsigned int)%s %s (unsigned int)(%s)" % (gpr(args[0]), gpr(args[1]),
                                                                        op, b))
        elif mnem in ("mult", "multu"):
            # HI/LO are never read back by the decompiled code; keep operand side effects only
            stmts.append("(void)%s; (void)%s" % (gpr(args[0]), gpr(args[1])))
        else:
            raise ValueError("unsupported asm instruction %r" % ins)
    for i, (c, e) in enumerate(inputs):
        if len(outputs) + i not in used:
            stmts.append("(void)(%s)" % e)
    decl = ""
    if temps:
        decl = "unsigned int %s; " % ", ".join("__r%d = 0" % t for t in sorted(temps))
    if not stmts:
        return "((void)0)"
    return "do { " + decl + "; ".join(stmts) + "; } while (0)"


def translate_asm(src):
    out = []
    i = 0
    clean = _strip_comments(src)
    while True:
        m = _ASM_RE.search(clean, i)
        if not m:
            out.append(src[i:])
            break
        # `register int x asm("$0")`: a register binding, not a statement.
        start = m.start()
        lp = m.end() - 1
        rp = _match_paren(src, lp)
        inner = src[lp + 1:rp - 1]
        parts = _split_top(inner, ":")
        out.append(src[i:start])
        if len(parts) == 1 and re.fullmatch(r'\s*"\$\w+"\s*', inner):
            # register binding: `register T v asm("$0")` means a zero register; others are hints
            out.append("= 0" if inner.strip() in ('"$0"', '"$zero"') else "")
        else:
            tmpl = parts[0]
            outs = _operand_exprs(parts[1]) if len(parts) > 1 else []
            ins = _operand_exprs(parts[2]) if len(parts) > 2 else []
            out.append(translate_asm_stmt(tmpl, outs, ins))
        i = rp
    return "".join(out)


# --------------------------------------------------------------------------------------------
# C++: egcs virtual-function ABI on top of non-virtual native classes

_CLASS_RE = re.compile(r"(?<![\w.])(class|struct)\s+(\w+)\s*(:\s*([^{;()]*))?\{")


class ClassInfo:
    def __init__(self, name, bases, body_start, body_end):
        self.name = name
        self.bases = bases
        self.body_start = body_start  # index just past '{'
        self.body_end = body_end  # index of the closing '}'
        self.own_virtuals = []  # method names, declaration order (dtor = '~')
        self.has_delete = False
        self.polymorphic = False
        self.root = False
        self.slots = []  # full vtable slot list (method names)


def _strip_comments(s):
    s = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), s, flags=re.S)
    s = re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), s)
    return s


def _top_level_decls(body):
    """Yield top-level declarations of a class body (skipping nested brace blocks' contents but
    keeping their header), as text chunks ending at ';' or at a '{...}' block."""
    i, n, start = 0, len(body), 0
    while i < n:
        c = body[i]
        if c == "{":
            j = _match_paren(body, i, "{", "}")
            yield body[start:j]
            i = j
            # skip optional ';' after an inline body
            k = _skip_ws_comments(body, i)
            if k < n and body[k] == ";":
                i = k + 1
            start = i
            continue
        if c == ";":
            yield body[start:i + 1]
            start = i + 1
        i += 1


def _virtual_name(decl, cls):
    d = decl.strip()
    d = re.sub(r"^(public|private|protected)\s*:\s*", "", d)
    if not re.search(r"\bvirtual\b", d):
        return None
    m = re.search(r"(~?\s*\w+|operator\s*\S+?)\s*\(", d)
    if not m:
        return None
    name = re.sub(r"\s+", "", m.group(1))
    return "~" if name.startswith("~") else name


def parse_classes(src):
    clean = _strip_comments(src)
    classes = {}
    order = []
    for m in _CLASS_RE.finditer(clean):
        name = m.group(2)
        brace = m.end() - 1
        try:
            end = _match_paren(clean, brace, "{", "}") - 1
        except ValueError:
            continue
        bases = []
        if m.group(4):
            for b in m.group(4).split(","):
                b = re.sub(r"\b(public|private|protected|virtual)\b", "", b).strip()
                if b:
                    bases.append(b)
        ci = ClassInfo(name, bases, brace + 1, end)
        body = clean[brace + 1:end]
        for decl in _top_level_decls(body):
            vn = _virtual_name(decl, name)
            if vn and vn not in ci.own_virtuals:
                ci.own_virtuals.append(vn)
            if re.search(r"\boperator\s+delete\b", decl):
                ci.has_delete = True
        if name in classes:  # forward-declared twice? keep the one with a body
            continue
        classes[name] = ci
        order.append(name)
    # polymorphism and slot layout, bases first
    def resolve(c, seen=()):
        if c.slots or c.name in seen:
            return
        base_slots = []
        poly_base = None
        for b in c.bases:
            bc = classes.get(b)
            if bc:
                resolve(bc, seen + (c.name,))
                if bc.polymorphic and poly_base is None:
                    poly_base = bc
        if poly_base:
            base_slots = list(poly_base.slots)
        slots = list(base_slots)
        for v in c.own_virtuals:
            # an override of an inherited virtual reuses the slot (a dtor always overrides)
            if v not in slots:
                slots.append(v)
        c.slots = slots
        c.polymorphic = bool(slots)
        c.root = c.polymorphic and poly_base is None
        c.vbase = poly_base.name if poly_base else None
    for n in order:
        resolve(classes[n])
    return classes, order


def _root_of(classes, c):
    while c and not c.root:
        c = classes.get(c.vbase)
    return c


_DELETE_RE = re.compile(r"(?<![\w.>])(?<!operator )(?<!operator)delete\s+(?![\[(])")


def mark_deletes(src):
    """egcs implements `delete p` by calling p's destructor with in_chrg 3, and the destructor
    frees through its class's operator delete. Native g++ calls the destructor and then an
    operator delete of its own choosing. Each delete statement records its pointer first
    (pc_delete_mark) so the build's destructor thunks pass in_chrg 3 for exactly that object, and
    the operator delete that follows is skipped (see link.py, "destructor thunks")."""
    clean = _strip_comments(src)
    out, i = [], 0
    for m in _DELETE_RE.finditer(clean):
        if m.start() < i:
            continue
        # the operand runs to the statement's ';' at depth 0
        j, depth = m.end(), 0
        while j < len(clean):
            c = clean[j]
            if c in "([":
                depth += 1
            elif c in ")]":
                depth -= 1
            elif c == ";" and depth == 0:
                break
            j += 1
        if j >= len(clean) or depth:
            continue
        expr = src[m.end():j]
        out.append(src[i:m.start()])
        out.append("pc_delete(%s);" % expr)
        i = j + 1
    out.append(src[i:])
    return "".join(out)


_CAST_INC_RE = re.compile(r"\*\s*\(\s*\(\s*([\w ]+?)\s*\*\s*\)\s*(\w+)\s*\)\s*\+\+")


def fix_cast_lvalues(src):
    """g++ 2.x's cast-as-lvalue `*((T *)p)++` (removed from modern GCC)."""
    return _CAST_INC_RE.sub(
        lambda m: "(*(((%s *)((%s) = (__typeof__(%s))((%s *)(%s) + 1))) - 1))" % (
            m.group(1), m.group(2), m.group(2), m.group(1), m.group(2)), src)


def strip_virtual_bodies(src):
    """Drop the in-class bodies of virtual member functions (not destructors): `virtual void AI() {}`
    becomes `virtual void AI();`. Once `virtual` is gone, g++ would inline such a body at every
    call and never dispatch; as a declaration the call goes through the retail vtable (thunk)."""
    clean = _strip_comments(src)
    edits = []
    for m in _CLASS_RE.finditer(clean):
        brace = m.end() - 1
        try:
            end = _match_paren(clean, brace, "{", "}") - 1
        except ValueError:
            continue
        for v in re.finditer(r"\bvirtual\b", clean[brace + 1:end]):
            i = brace + 1 + v.start()
            lp = clean.find("(", i)
            semi = clean.find(";", i)
            if lp < 0 or lp > end or (0 <= semi < lp) or "~" in clean[i:lp]:
                continue
            k = _skip_ws_comments(clean, _match_paren(clean, lp))
            while k < end and clean[k] not in "{;":  # const, throw(), ...
                k += 1
            if k < end and clean[k] == "{":
                edits.append((k, _match_paren(clean, k, "{", "}")))
    for a, b in sorted(edits, reverse=True):
        src = src[:a] + ";" + src[b:]
    return src


def transform_cxx(src, vtables, tukey="tu"):
    """vtables: {class name: its vtable (a global of game/data)}. Returns (new source, class info dict for the
    build: {class: {"vptr_root": root class, "slots": [...], "has_delete": bool}})."""
    src = re.sub(r"^[ \t]*#[ \t]*pragma[ \t]+(interface|implementation)\b[^\n*]*$", "", src, flags=re.M)
    src = strip_virtual_bodies(src)
    classes, order = parse_classes(src)
    clean = _strip_comments(src)
    info = {}
    edits = []  # (pos, text_to_insert)
    for n in order:
        c = classes[n]
        if not c.polymorphic:
            continue
        root = _root_of(classes, c)
        info[n] = {"root": root.name if root else n, "slots": c.slots,
                   "has_delete": c.has_delete, "bases": c.bases}
        if c.root:
            edits.append((c.body_end, "\npublic: void *__vptr;\n"))
        is_template = re.search(r"template\s*<[^<>]*>\s*$", clean[:c.body_start].rsplit(";", 1)[-1]
                                .rsplit("}", 1)[-1].split("class " + n)[0].split("struct " + n)[0])
        if "~" in c.slots and root and not is_template:
            # egcs deletes through the vtable destructor with in_chrg 3 (see pc_game.h)
            semi = clean.find(";", c.body_end)
            r = root.name
            edits.append((semi + 1,
                          "\nstatic inline void pc_delete(%s *p) { if (p) pc_vdelete("
                          "static_cast<%s *>(p)->__vptr, %d, static_cast<%s *>(p)); }\n"
                          % (n, r, c.slots.index("~"), r)))
    # constructors / destructors of polymorphic classes defined in this TU: store the vtable (the
    # class's vtable, a global of game/data)
    clean = _strip_comments(src)
    used_vtables = set()
    for n, c in classes.items():
        if not c.polymorphic or n not in vtables:
            continue
        store = " this->__vptr = (void *)pcvt_%s; " % vtables[n]
        used_vtables.add(vtables[n])
        # out-of-line: `Class::Class(...) [: inits] {` and `Class::~Class() {`
        for m in re.finditer(r"\b%s\s*::\s*(~\s*)?%s\s*\(" % (n, n), clean):
            lp = m.end() - 1
            rp = _match_paren(clean, lp)
            k = _skip_ws_comments(clean, rp)
            # skip a member-initializer list up to the body
            if k < len(clean) and clean[k] == ":":
                depth = 0
                while k < len(clean):
                    ch = clean[k]
                    if ch in "([":
                        depth += 1
                    elif ch in ")]":
                        depth -= 1
                    elif ch == "{" and depth == 0:
                        break
                    k += 1
            if k < len(clean) and clean[k] == "{":
                edits.append((k + 1, store))
        # in-class definitions: `Class(...) [: inits] {` / `~Class() {` inside the body
        body = clean[c.body_start:c.body_end]
        for m in re.finditer(r"(?<![\w:~])(~\s*)?%s\s*\(" % n, body):
            if m.start() > 0 and body[m.start() - 1] in ".>":
                continue
            lp = c.body_start + m.end() - 1
            try:
                rp = _match_paren(clean, lp)
            except ValueError:
                continue
            k = _skip_ws_comments(clean, rp)
            if k < len(clean) and clean[k] == ":":
                depth = 0
                while k < len(clean):
                    ch = clean[k]
                    if ch in "([":
                        depth += 1
                    elif ch in ")]":
                        depth -= 1
                    elif ch == "{" and depth == 0:
                        break
                    elif ch == ";" and depth == 0:
                        break
                    k += 1
            if k < len(clean) and clean[k] == "{":
                edits.append((k + 1, store))
    # (under a name of their own, whatever the source declares: the game TUs are compiled with a
    # '_' before every C name, build.py)
    for pos, text in sorted(edits, key=lambda e: -e[0]):
        src = src[:pos] + text + src[pos:]
    src = "".join('extern "C" char pcvt_%s[] __asm__("_%s");\n' % (v, v)
                  for v in sorted(used_vtables)) + src
    # virtual off; pure virtuals become plain declarations (only after `virtual`: a statement like
    # `SCRATCH_S(0x2d4) = 0;` has the same shape)
    src = re.sub(r"\bvirtual\b([^;{}]*\))\s*(const\s*)?=\s*0\s*;",
                 lambda m: "%s %s;" % (m.group(1), m.group(2) or ""), src)
    src = re.sub(r"\bvirtual\b", "", src)
    # the vptr offset of each root class, for the build's virtual-dispatch thunks
    roots = sorted({v["root"] for v in info.values()})
    if roots:
        src += '\nextern "C" {\n'
        for r in roots:
            src += "int pcvptroff_%s_%s = __builtin_offsetof(%s, __vptr);\n" % (tukey, r, r)
        src += "}\n"
    return src, info
