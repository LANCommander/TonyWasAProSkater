"""Give a decompiled data table its C++ type: rewrite `PSX_DATA(addr) int Name[n] = {...};` in
decomp/data as `PSX_DATA(addr) Type Name[n] = {...};`, a typed initialiser with string literals
for the text it points at.

    python tools/type_data.py Name 'Type' [--include game/x.h] [--dry]

Type is a C++ type with its dimensions ('SLevel[10]', 'LocalText', 'char *[25]'), from the
headers --include names (decomp/include) or psxdata.h. The table's bytes come from the data as it
is (platform/pc/tools/data_image.py), its layout from the DWARF of a probe compiled with those
headers, so the rewrite can't change the data: data_image.py --check still compares it with the
retail bytes. A `char *` (or `unsigned char *`) that points at text the code doesn't write to
becomes the string literal; other pointers name their target (cast to the field's type).
"""
import argparse
import os
import re
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DECOMP = os.path.dirname(HERE)
ROOT = os.path.dirname(DECOMP)
DATA = os.path.join(DECOMP, "data")
sys.path.insert(0, os.path.join(ROOT, "platform", "pc", "tools"))
import data_image  # noqa: E402
import toolchain  # noqa: E402
from layouts import Layouts, intval, parse_dwarf, ref  # noqa: E402

DEF_RE = r"^PSX_DATA\((0x[0-9a-f]+)\) [^=;\n]*?\b%s\b[^=;\n]*(?: = \{.*?^\}| = [^{\n][^\n]*?|)\s*;\n"
STR_DEF = re.compile(r'^PSX_DATA\((0x[0-9a-f]+)\) char (\w+)\[\d*\] = ("(?:[^"\\\n]|\\.)*");', re.M)
# text buffers the game writes to: pointers to them stay pointers
WRITABLE = {"SaveGameName", "CARD_FNAME", "CARD_VNAME", "s_800a42dc", "s_800a42e8"}
BSS_START = 0x800D21C8


def c_string(b):
    out = ""
    for i, c in enumerate(b):
        ch = chr(c)
        if ch in '"\\':
            out += "\\" + ch
        elif ch == "?" and i + 1 < len(b) and chr(b[i + 1]) == "?":
            out += "\\?"  # no trigraphs
        elif 32 <= c < 127:
            # a hex escape runs on into following hex digits: end it with "" first
            if out.endswith(tuple("\\x%02x" % k for k in range(256))) and ch in "0123456789abcdefABCDEF":
                out += '""'
            out += ch
        elif c == 10:
            out += "\\n"
        elif c == 9:
            out += "\\t"
        else:
            out += "\\x%02x" % c
    return '"%s"' % out


def number(v):
    """Decimal, but hex for a mask-like or big value (0x10000, 0x7fffffff)."""
    m = abs(v)
    if m < 0x1000 or (m < 0x1000000 and len(("%x" % m).rstrip("0")) > 2):
        return str(v)
    return "0x%x" % v if v >= 0 else "-0x%x" % m


def is_text(b):
    return all(32 <= c < 127 or c in (9, 10) or c >= 0x80 for c in b)


class Data:
    def __init__(self):
        toolchain.add_to_path()
        self.lay = data_image.build(data_image.OBJDIR)
        self.by_addr = {g["addr"]: g for g in self.lay.globals}
        self.by_name = {g["name"]: g for g in self.lay.globals}
        self.files = {}
        for f in sorted(os.listdir(DATA)):
            if f.endswith(".cpp"):
                self.files[f] = open(os.path.join(DATA, f), encoding="utf-8").read()
        # text globals: their literal as written
        self.text = {}
        for t in self.files.values():
            for m in STR_DEF.finditer(t):
                self.text[int(m.group(1), 16)] = m.group(3)
        # names the decompiled code uses: what they point at stays a global
        used = set()
        for dp, _, fs in os.walk(os.path.join(DECOMP, "src")):
            for f in fs:
                if f.endswith((".c", ".cpp", ".h")):
                    used.update(re.findall(r"\b\w+\b", open(os.path.join(dp, f), encoding="utf-8",
                                                           errors="replace").read()))
        self.code_names = used
        self.names = data_image.name_table()

    def global_at(self, a):
        g = self.by_addr.get(a)
        if g:
            return g
        for g in self.lay.globals:
            if g["addr"] <= a < g["end"]:
                return g
        return None

    def bytes_at(self, a, n):
        out = bytearray()
        for k in range(n):
            g = self.global_at(a + k)
            out.append(g["data"][a + k - g["addr"]] if g else 0)
        return bytes(out)

    def literal(self, a):
        """The string literal for a pointer to a, if a holds text nothing writes to, else None."""
        if not data_image.DATA_START <= a < BSS_START:  # code, .bss
            return None
        g = self.global_at(a)
        if g and (g["name"] in WRITABLE or g["name"] in self.code_names):
            return None
        if a in self.text and g and g["addr"] == a:
            return self.text[a]
        if g is None:
            # the padding after a string: an empty string
            prev = max((x for x in self.lay.globals if x["end"] <= a), key=lambda x: x["end"],
                       default=None)
            if prev and prev["addr"] in self.text and a - prev["end"] < 4:
                return '""'
            return None
        if g["addr"] == a and len(g["data"]) <= 4:
            # a 1-character string kept as a word (`int D_800d1404[1]`)
            b = self.bytes_at(a, 4)
            s = b[:b.index(0)] if 0 in b else None
            if s is not None and is_text(s):
                return c_string(s)
        return None

    def name_of(self, a):
        """A C++ expression for the address a: a global's name (+ offset), else the number."""
        g = self.global_at(a)
        if g:
            return g["name"], a - g["addr"]
        cand = [n for n, s in self.names.items() if a in s and len(s) == 1]
        if cand:
            return sorted(cand, key=lambda n: (n.startswith(("D_", "DAT_", "FUN_")), n))[0], 0
        return None, a


class Types:
    """The DWARF of `Type probe;` compiled with the headers."""

    def __init__(self, ctype, includes):
        base, dims = re.match(r"^(.*?)((?:\[\d*\])*)$", ctype.strip()).groups()
        src = '#include "psxdata.h"\n' + "".join('#include "%s"\n' % h for h in includes) + \
              "%s probe%s;\n" % (base, dims)
        d = tempfile.mkdtemp()
        cpp, obj = os.path.join(d, "probe.cpp"), os.path.join(d, "probe.o")
        open(cpp, "w").write(src)
        r = subprocess.run([toolchain.tool("g++"), "-m32", "-std=gnu++98", "-g", "-w", "-fno-exceptions",
                            "-funsigned-char", "-I", DATA] + data_image.INCLUDES + [
                            "-c", cpp, "-o", obj], capture_output=True, text=True)
        if r.returncode:
            raise SystemExit("type_data: %s\n%s" % (ctype, r.stderr[-3000:]))
        dump = subprocess.run([toolchain.tool("objdump"), "--dwarf=info", obj],
                              capture_output=True, text=True).stdout
        self.dies = parse_dwarf(dump)
        self.L = Layouts(self.dies)
        var = next(x for x in self.dies.values() if x["tag"] == "DW_TAG_variable" and
                   x["attrs"].get("DW_AT_name", "").split()[-1] == "probe")
        self.root = self.dies[ref(var["attrs"]["DW_AT_type"])]
        self.base, self.dims = base, dims

    def strip(self, die):
        return self.L.strip(die["off"])

    def spell(self, die):
        """C++ spelling of a type, for a cast; None if it can't be spelled simply."""
        if die is None:
            return "void"
        t, a = die["tag"], die["attrs"]
        name = a.get("DW_AT_name", "").split()[-1] if "DW_AT_name" in a else None
        if t in ("DW_TAG_base_type", "DW_TAG_typedef", "DW_TAG_structure_type",
                 "DW_TAG_class_type", "DW_TAG_union_type", "DW_TAG_enumeration_type"):
            if t == "DW_TAG_base_type":
                return a["DW_AT_name"].split(":")[-1].strip()
            return name
        if t == "DW_TAG_pointer_type":
            inner = self.dies.get(ref(a.get("DW_AT_type"))) if "DW_AT_type" in a else None
            s = self.spell(inner)
            return None if s is None else s + " *"
        if t in ("DW_TAG_const_type", "DW_TAG_volatile_type"):
            inner = self.dies.get(ref(a.get("DW_AT_type")))
            s = self.spell(inner)
            return None if s is None else ("const " if "const" in t else "volatile ") + s
        return None


class Emit:
    def __init__(self, data, types, g):
        self.d, self.t, self.g = data, types, g
        self.ptrs = {off: (t, v) for off, t, v in g["ptrs"]}
        self.lits = dict(g["lits"])

    def word(self, off, n):
        b = self.g["data"][off:off + n]
        b = b + bytes(n - len(b))
        return int.from_bytes(b, "little")

    def scalar(self, die, off, path):
        a, t = die["attrs"], die["tag"]
        size = self.t.L.size(die)
        if t in ("DW_TAG_pointer_type",):
            return self.pointer(die, off, path)
        if off in self.ptrs or off in self.lits:
            raise SystemExit("type_data: a pointer at +0x%x (%s), which the type says isn't one" % (off, path))
        v = self.word(off, size)
        enc = a.get("DW_AT_encoding", "") if t == "DW_TAG_base_type" else ""
        signed = t == "DW_TAG_enumeration_type" or ("signed" in enc and "unsigned" not in enc)
        if t == "DW_TAG_base_type" and a.get("DW_AT_name", "").split()[-1] == "char":
            signed = False  # -funsigned-char, as the game
        if signed and v >> (8 * size - 1):
            v -= 1 << (8 * size)
        if t == "DW_TAG_enumeration_type":
            return "(%s)%d" % (self.t.spell(die), v)
        if "float" in enc:
            return repr(struct.unpack("<f" if size == 4 else "<d", self.g["data"][off:off + size])[0])
        if re.search(r"flag|mask", path.split(".")[-1], re.I) and v > 0xff:
            return "0x%x" % (v & ((1 << 8 * size) - 1))
        return number(v)

    def pointer(self, die, off, path):
        if off in self.lits:
            return c_string(self.lits[off])
        inner = self.t.dies.get(ref(die["attrs"].get("DW_AT_type"))) if "DW_AT_type" in die["attrs"] else None
        inner_s = self.t.L.strip(inner["off"]) if inner else None
        charp = inner_s is not None and inner_s["tag"] == "DW_TAG_base_type" and \
            self.t.L.size(inner_s) == 1
        cast = self.t.spell(die)
        if cast is None:
            cast = "__typeof__(((%s *)0)->%s)" % (self.t.base, path)
        if off not in self.ptrs:
            v = self.word(off, 4)
            return "0" if v == 0 else "(%s)0x%x" % (cast, v)
        t, v = self.ptrs[off]
        if charp and v == 0:
            lit = self.d.literal(t)
            if lit is not None:
                return lit
        name, k = self.d.name_of(t)
        if name is None:
            return "(%s)0x%x" % (cast, (t + v) & 0xFFFFFFFF)
        self.needed.add(name)
        k += v
        if k:
            return "(%s)((char *)%s + %d)" % (cast, name, k)
        return "(%s)%s" % (cast, name)

    def value(self, die, off, path, depth):
        die = self.t.strip(die)
        tag = die["tag"]
        if tag in ("DW_TAG_structure_type", "DW_TAG_class_type", "DW_TAG_union_type"):
            if any(c["tag"] == "DW_TAG_inheritance" for c in die["children"]):
                raise SystemExit("type_data: %s has a base class: no aggregate initialiser" % path)
            members = [c for c in die["children"] if c["tag"] == "DW_TAG_member" and
                       "DW_AT_data_member_location" in c["attrs"]]
            if tag == "DW_TAG_union_type":
                members = members[:1]
            vals = []
            for c in members:
                if "DW_AT_bit_size" in c["attrs"]:
                    raise SystemExit("type_data: bit-fields (%s)" % path)
                mo = intval(c["attrs"]["DW_AT_data_member_location"])
                nm = c["attrs"]["DW_AT_name"].split()[-1]
                vals.append(self.value(self.t.dies[ref(c["attrs"]["DW_AT_type"])], off + mo,
                                       "%s.%s" % (path, nm) if path else nm, depth + 1))
            return self.agg(vals, depth)
        if tag == "DW_TAG_array_type":
            el = self.t.dies[ref(die["attrs"]["DW_AT_type"])]
            el_s = self.t.strip(el)
            dims = [intval(c["attrs"].get("DW_AT_count")) if "DW_AT_count" in c["attrs"] else
                    intval(c["attrs"].get("DW_AT_upper_bound", "-1")) + 1
                    for c in die["children"] if c["tag"] == "DW_TAG_subrange_type"]
            esz = self.t.L.size(el)
            if len(dims) > 1:  # T x[a][b]: an array of arrays
                inner = dict(die)
                inner["children"] = [c for c in die["children"] if c["tag"] == "DW_TAG_subrange_type"][1:]
                isz = esz
                for n in dims[1:]:
                    isz *= n
                vals = [self.value_arr(inner, el, off + i * isz, "%s[%d]" % (path, i), depth + 1)
                        for i in range(dims[0])]
                return self.agg(vals, depth)
            n = dims[0]
            if el_s["tag"] == "DW_TAG_base_type" and esz == 1 and not any(
                    off + i in self.ptrs or off + i in self.lits for i in range(n)):
                b = self.g["data"][off:off + n]
                b = b + bytes(n - len(b))
                s = b.rstrip(b"\0")
                if s and is_text(s) and len(s) < n and 0 not in s:
                    return c_string(s)
            vals = [self.value(el, off + i * esz, "%s[%d]" % (path, i), depth + 1) for i in range(n)]
            return self.agg(vals, depth)
        return self.scalar(die, off, path)

    def value_arr(self, arrdie, el, off, path, depth):
        sub = {"tag": "DW_TAG_array_type", "attrs": {"DW_AT_type": "<0x%x>" % el["off"]},
               "children": arrdie["children"], "off": -1}
        self.t.dies[-1] = sub
        return self.value(sub, off, path, depth)

    @staticmethod
    def zero(v):
        return v in ("0", "{}") or re.fullmatch(r"\(.*\)0", v) is not None

    def agg(self, vals, depth):
        while vals and self.zero(vals[-1]):
            vals.pop()
        return "{%s}" % ", ".join(vals) if vals else "{}"

    def top(self):
        self.needed = set()
        root = self.t.root
        rs = self.t.strip(root)
        if rs["tag"] == "DW_TAG_array_type" and self.t.dims:
            el = self.t.dies[ref(rs["attrs"]["DW_AT_type"])]
            esz = self.t.L.size(el)
            n = self.t.L.count(rs)
            vals = [self.value(el, i * esz, "[%d]" % i, 1) for i in range(n)]
            while vals and self.zero(vals[-1]):
                vals.pop()
            return vals, True
        v = self.value(root, 0, "", 0)
        return v, False


def wrap(items, indent="\t", width=100):
    """Initialiser items, as many to a line as fit."""
    lines, cur = [], indent
    for it in items:
        piece = it + ","
        if cur.strip() and len(cur) + 1 + len(piece) > width:
            lines.append(cur.rstrip())
            cur = indent
        cur += (" " if cur.strip() else "") + piece
    if cur.strip():
        lines.append(cur.rstrip())
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("type")
    ap.add_argument("--include", action="append", default=[])
    ap.add_argument("--dry", action="store_true")
    a = ap.parse_args()
    d = Data()
    g = d.by_name.get(a.name)
    if g is None:
        raise SystemExit("type_data: no global %s" % a.name)
    t = Types(a.type, a.include)
    size = t.L.size(t.root)
    if size != g["end"] - g["addr"] and not (size > g["end"] - g["addr"] and
                                             not any(g["data"][size:])):
        print("type_data: note: %s is %d bytes, the global %d" % (a.type, size, g["end"] - g["addr"]))
    e = Emit(d, t, g)
    vals, is_array = e.top()
    if is_array:
        body = "{\n" + "\n".join("\t%s," % v if len(v) < 96 else wrap_elem(v) for v in vals) + "\n}"
    elif not vals.startswith("{"):
        body = vals
    else:
        inner = vals[1:-1]
        items = split_top(inner)
        body = "{\n" + wrap(items) + "\n}"
    decl = "PSX_DATA(0x%08x) %s %s%s = %s;\n" % (g["addr"], t.base.rstrip(), a.name, t.dims, body)
    decl = decl.replace("* %s" % a.name, "*%s" % a.name)
    fname = next(f for f, txt in d.files.items() if re.search(DEF_RE % re.escape(a.name), txt, re.M | re.S))
    txt = d.files[fname]
    m = re.search(DEF_RE % re.escape(a.name), txt, re.M | re.S)
    txt = txt[:m.start()] + decl + txt[m.end():]
    for h in a.include:
        inc = '#include "%s"\n' % h
        if inc not in txt:
            txt = txt.replace('#include "psxdata.h"\n', '#include "psxdata.h"\n' + inc, 1)
    # what the new initialiser names that the file doesn't declare
    missing = sorted(n for n in e.needed if not re.search(r"\b%s\s*\[" % re.escape(n), txt))
    if missing:
        i = txt.index("\n\n", txt.index('#include "psxdata.h"')) + 1
        txt = txt[:i] + "".join("extern char %s[];\n" % n for n in missing) + txt[i:]
    if a.dry:
        print(decl)
        return 0
    open(os.path.join(DATA, fname), "w", encoding="utf-8", newline="\n").write(txt)
    print("%s: %s %s%s" % (fname, t.base, a.name, t.dims))
    return 0


def split_top(s):
    """Split an initialiser list's text at its top-level commas."""
    out, depth, cur, q = [], 0, "", False
    i = 0
    while i < len(s):
        c = s[i]
        if q:
            cur += c
            if c == "\\":
                cur += s[i + 1]
                i += 1
            elif c == '"':
                q = False
        elif c == '"':
            q = True
            cur += c
        elif c in "{(":
            depth += 1
            cur += c
        elif c in "})":
            depth -= 1
            cur += c
        elif c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    if cur.strip():
        out.append(cur.strip())
    return out


def wrap_elem(v):
    """One array element that doesn't fit a line: its fields wrapped, one level in."""
    items = split_top(v[1:-1])
    return "\t{\n" + wrap(items, "\t\t") + "\n\t},"


if __name__ == "__main__":
    sys.exit(main())
