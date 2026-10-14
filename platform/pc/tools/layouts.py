#!/usr/bin/env python3
"""Snapshot and compare the class layouts each C++ TU of the port compiles with.

    python platform/pc/tools/layouts.py snapshot OUT.json [--only SUBSTR]
    python platform/pc/tools/layouts.py compare BASE.json [--only SUBSTR]

Reads the transformed sources build.py leaves in build/pc/obj (<tu>.port.cpp: egcs vptrs already
inserted), recompiles each with debug info and flattens every named class/struct to its scalar
leaves: (offset, size, kind), where kind is the canonical scalar type (s8/u8/s16/u16/s32/u32/ptr/...).
Inherited members, nested structs and small scalar arrays are flattened too, so `CVector mPos` and
`int px, py, pz` compare equal.

`compare` checks each TU against the snapshot: every leaf a class had (padding members excluded)
must still be there at the same offset with the same size (error otherwise), and with the same
signedness/pointer kind (warning: the code reading it may now behave differently). Used to move
local class declarations into shared headers without changing any layout.
"""
import argparse
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build  # noqa: E402

PAD_RE = re.compile(r"^(pad|_?unk|_[0-9a-fA-F]|__pad|filler)", re.I)
MAX_ARRAY_FLATTEN = 64


def parse_dwarf(text):
    """Parse `objdump --dwarf=info` into {offset: die}; die = {tag, attrs, children, depth}."""
    dies, stack = {}, []
    cur = None
    for line in text.splitlines():
        m = re.match(r"\s*<(\d+)><([0-9a-f]+)>: Abbrev Number: (\d+)(?: \((\w+)\))?", line)
        if m:
            depth, off = int(m.group(1)), int(m.group(2), 16)
            if m.group(3) == "0":
                cur = None
                continue
            cur = {"tag": m.group(4), "attrs": {}, "children": [], "off": off}
            dies[off] = cur
            while len(stack) > depth:
                stack.pop()
            if stack:
                stack[-1]["children"].append(cur)
            stack.append(cur)
            continue
        m = re.match(r"\s*<[0-9a-f]+>\s+(DW_AT_\w+)\s*:\s*(.*)$", line)
        if m and cur is not None:
            k, v = m.group(1), m.group(2).strip()
            v = re.sub(r"^\(indirect (line )?string, offset: (?:0x)?[0-9a-f]+\):\s*", "", v)
            cur["attrs"][k] = v
    return dies


def ref(v):
    m = re.match(r"<0x([0-9a-f]+)>", v or "")
    return int(m.group(1), 16) if m else None


def intval(v):
    m = re.match(r"(0x[0-9a-f]+|-?\d+)", v or "")
    return int(m.group(1), 0) if m else None


class Layouts:
    def __init__(self, dies):
        self.d = dies
        self.cache = {}

    def strip(self, off):
        """Follow typedef/const/volatile."""
        seen = 0
        while off is not None and seen < 50:
            die = self.d.get(off)
            if not die or die["tag"] not in ("DW_TAG_typedef", "DW_TAG_const_type",
                                             "DW_TAG_volatile_type"):
                return die
            off = ref(die["attrs"].get("DW_AT_type"))
            seen += 1
        return None

    def size(self, die):
        if die is None:
            return 0
        s = intval(die["attrs"].get("DW_AT_byte_size"))
        if s is not None:
            return s
        if die["tag"] == "DW_TAG_array_type":
            el = self.strip(ref(die["attrs"].get("DW_AT_type")))
            return self.size(el) * self.count(die)
        if die["tag"] in ("DW_TAG_pointer_type", "DW_TAG_reference_type"):
            return 4
        return 0

    def count(self, arr):
        n = 1
        for c in arr["children"]:
            if c["tag"] == "DW_TAG_subrange_type":
                ub = intval(c["attrs"].get("DW_AT_upper_bound"))
                cnt = intval(c["attrs"].get("DW_AT_count"))
                n *= cnt if cnt is not None else (ub + 1 if ub is not None else 0)
        return n

    def kind(self, die):
        if die is None:
            return "?"
        t = die["tag"]
        if t in ("DW_TAG_pointer_type", "DW_TAG_reference_type", "DW_TAG_ptr_to_member_type"):
            return "ptr"
        if t == "DW_TAG_enumeration_type":
            return "enum%d" % (self.size(die) * 8)
        if t == "DW_TAG_base_type":
            enc = die["attrs"].get("DW_AT_encoding", "")
            sz = self.size(die) * 8
            if "float" in enc:
                return "f%d" % sz
            if "boolean" in enc:
                return "bool"
            if "unsigned" in enc:
                return "u%d" % sz
            if "signed char" in enc and die["attrs"].get("DW_AT_name") == "char":
                return "s%d" % sz  # plain char is signed on both targets' ABIs here
            return "s%d" % sz
        return t

    def leaves(self, die, base=0, name="", depth=0):
        """Yield (offset, size, kind, name, is_pad) scalar leaves of a type."""
        die = self.strip(die["off"]) if die else None
        if die is None or depth > 12:
            return
        t = die["tag"]
        if t in ("DW_TAG_structure_type", "DW_TAG_class_type"):
            for c in die["children"]:
                if c["tag"] == "DW_TAG_inheritance":
                    o = intval(c["attrs"].get("DW_AT_data_member_location")) or 0
                    yield from self.leaves(self.d.get(ref(c["attrs"].get("DW_AT_type"))),
                                           base + o, name, depth + 1)
                elif c["tag"] == "DW_TAG_member" and "DW_AT_data_member_location" in c["attrs"]:
                    if "DW_AT_bit_size" in c["attrs"] or "DW_AT_data_bit_offset" in c["attrs"]:
                        continue
                    o = intval(c["attrs"]["DW_AT_data_member_location"]) or 0
                    n = c["attrs"].get("DW_AT_name", "")
                    sub = self.d.get(ref(c["attrs"].get("DW_AT_type")))
                    if PAD_RE.match(n):
                        yield (base + o, self.size(self.strip(sub["off"]) if sub else None),
                               "pad", name + "." + n, True)
                        continue
                    yield from self.leaves(sub, base + o, name + "." + n, depth + 1)
            return
        if t == "DW_TAG_union_type":
            yield (base, self.size(die), "union", name, False)
            return
        if t == "DW_TAG_array_type":
            el = self.strip(ref(die["attrs"].get("DW_AT_type")))
            n, es = self.count(die), self.size(el)
            if el is not None and n <= MAX_ARRAY_FLATTEN and es:
                for i in range(n):
                    yield from self.leaves(el, base + i * es, "%s[%d]" % (name, i), depth + 1)
            else:
                yield (base, n * es, "array:%s" % self.kind(el), name, False)
            return
        yield (base, self.size(die), self.kind(die), name, False)

    def classes(self):
        out = {}
        for die in self.d.values():
            if die["tag"] not in ("DW_TAG_structure_type", "DW_TAG_class_type"):
                continue
            n = die["attrs"].get("DW_AT_name")
            if not n or "DW_AT_declaration" in die["attrs"] or n.startswith("._"):
                continue
            lv = sorted(set(self.leaves(die)))
            if n not in out or len(lv) > len(out[n]["leaves"]):
                out[n] = {"size": self.size(die), "leaves": [list(x) for x in lv]}
        return out


def dump_tu(src):
    base = build.tu_paths({"src": src})
    port = base + ".port.cpp"
    if not os.path.exists(port):
        return src, None, "no %s (build first)" % port
    obj = base + ".layout.o"
    cmd = [build.tool("g++")] + build.COMMON_FLAGS + build.CXX_FLAGS + [
        "-fno-merge-debug-strings", "-gdwarf-4", "-c", port, "-o", obj]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        return src, None, "compile failed: " + r.stderr[-1500:]
    r = subprocess.run([build.tool("objdump"), "--dwarf=info", obj], capture_output=True,
                       text=True, errors="replace")
    os.remove(obj)
    return src, Layouts(parse_dwarf(r.stdout)).classes(), None


def cxx_tus(only):
    out = []
    for p in build.read_tsv(os.path.join(build.GAME, "tus.tsv")):
        if p[0].endswith(".cpp") and p[0] not in out and (not only or only in p[0]):
            out.append(p[0])
    return out


def dump_all(only):
    os.environ["PATH"] = build.TOOLBIN + os.pathsep + os.environ["PATH"]
    res, errs = {}, {}
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        for src, cl, err in ex.map(dump_tu, cxx_tus(only)):
            if err:
                errs[src] = err
            else:
                res[src] = cl
    return res, errs


def compare(base, cur):
    """-> (errors, warnings): lists of strings."""
    errors, warnings = [], []
    for src, classes in sorted(base.items()):
        if src not in cur:
            continue
        now = cur[src]
        for cls, info in sorted(classes.items()):
            if cls not in now:
                continue  # no longer used by this TU: nothing to keep consistent
            have = {}
            for o, s, k, n, pad in now[cls]["leaves"]:
                have.setdefault(o, []).append((s, k, n, pad))
            for o, s, k, n, pad in info["leaves"]:
                if pad:
                    continue
                cands = [c for c in have.get(o, []) if not c[3]]
                same = [c for c in cands if c[0] == s]
                if not same:
                    got = ", ".join("%s %s(%d)" % (c[2] or "?", c[1], c[0]) for c in have.get(o, [])) or "nothing"
                    errors.append("%s: %s+0x%x %s %s(%d) -> %s" % (src, cls, o, n, k, s, got))
                elif not any(c[1] == k for c in same):
                    warnings.append("%s: %s+0x%x %s %s -> %s %s" % (src, cls, o, n, k, same[0][2],
                                                                     same[0][1]))
    return errors, warnings


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["snapshot", "compare"])
    ap.add_argument("path")
    ap.add_argument("--only", default=None)
    a = ap.parse_args()
    res, errs = dump_all(a.only)
    for s, e in sorted(errs.items()):
        print("SKIP %s: %s" % (s, e.splitlines()[0] if e else e))
    if a.mode == "snapshot":
        old = json.load(open(a.path)) if a.only and os.path.exists(a.path) else {}
        old.update(res)
        json.dump(old, open(a.path, "w"), indent=0)
        print("%d TUs, %d classes -> %s" % (len(res), sum(len(v) for v in res.values()), a.path))
        return 0
    errors, warnings = compare(json.load(open(a.path)), res)
    for w in warnings:
        print("warn  " + w)
    for e in errors:
        print("ERROR " + e)
    print("%d TUs compared: %d layout errors, %d type warnings" % (len(res), len(errors), len(warnings)))
    return 1 if errors or errs else 0


if __name__ == "__main__":
    sys.exit(main())
