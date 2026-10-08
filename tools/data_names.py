#!/usr/bin/env python3
"""
Name retail globals (the decomp's DAT_xxxxxxxx placeholders) from the THPS2 demo's MAIN.SYM.

A demo twin has the same code as its retail function, so an aligned lui/addiu|load|store pair (or a
gp-relative access) in both reaches the same global. The demo SYM names the demo address; every
twin casts a vote for that name at the retail address, and a name is kept only when the votes agree.

  python tools/data_names.py        writes decomp/data_names.tsv, prints a census

Twins: the demo-named rows of decomp/functions.tsv, build/matches.json's exact matches and
build/fuzzy_twins.tsv, aligned by opcode (difflib) so twins of different sizes work too. A demo
address resolves to a typed SYM global (extern/static def record, with C type and size), else a bare
SYM label, else an offset inside a typed object (`Base_<hex offset>`, the offset taken from Base's
own retail address when that's known). Static members `_7CCamera.TweakCol` become `CCamera_TweakCol`,
function-local statics `old_camDist.394` become `old_camDist`; other non-identifiers are left out.

Two more passes name what no twin reaches:
  layout  the demo and retail data sections keep each module's variable order. Between two exact
          names that are the same distance apart in both, every demo symbol keeps its offset (a
          leave-one-out test on the exact names: 294/294 right; one neighbour alone is unreliable).
  string  a referenced, still unnamed address holding a C string gets `s_<text>` (Ghidra style).
"""
import bisect
import collections
import difflib
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
from fuzzy_twins import opkey  # noqa: E402
from sym_locals import records, ctype  # noqa: E402
from parse_psyq_sym import BASE_TYPE  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..")
P = lambda *a: os.path.join(ROOT, *a)  # noqa: E731
DEMO_BASE, RETAIL_BASE = 0x80018000, 0x80010000
DEMO_GP, RETAIL_GP = 0x800E0140, 0x800D1080  # __SN_GP_BASE; retail's from the startup code
AGREE = 0.75          # share of a retail address's votes the winning demo address needs
MIN_ALIGN = 0.5       # share of the shorter twin covered by matching opcode blocks
MIN_BLOCK = 4         # matching run (words) a vote must sit in; shorter runs disagree ~20% of the time
IDENT = re.compile(r"^[A-Za-z_]\w*$")
SECTION = re.compile(r"^_+\w*?_(obj|objend|org|orgend|size)$|^__SN_")

sym = json.load(open(P("build", "MAIN_sym.json")))
demo = open(P("extern", "demo", "PROSKATE.EXE"), "rb").read()
retail = open(P("extern", "thps1_retail", "SLUS_008.60"), "rb").read()
RETAIL_END = RETAIL_BASE + len(retail) - 0x800      # initialised data ends here; .bss follows
GAME_CODE_END = 0x800858AC                           # end of game .text (the Psy-Q libraries follow)
SCALAR = {"char": 1, "uchar": 1, "short": 2, "ushort": 2, "int": 4, "uint": 4, "long": 4, "ulong": 4,
          "float": 4, "double": 8, "enum": 4}


def words(b, base, a, n):
    return struct.unpack_from(f"<{n // 4}I", b, a - base + 0x800)


def sx(i):
    return i - 0x10000 if i & 0x8000 else i


LOADSTORE = {32, 33, 34, 35, 36, 37, 38, 40, 41, 42, 43, 46, 50, 58}   # incl. lwc2/swc2


def data_refs(w, gp):
    """word index -> absolute address for each %lo / gp-relative access (lui tracked linearly)."""
    hi, out = {}, {}
    for i, x in enumerate(w):
        op, rs, rt, rd = x >> 26, (x >> 21) & 31, (x >> 16) & 31, (x >> 11) & 31
        imm = x & 0xFFFF
        if op == 15:
            hi[rt] = imm << 16
            continue
        if op == 9 or op in LOADSTORE:
            if rs in hi:
                out[i] = (hi[rs] + sx(imm)) & 0xFFFFFFFF
            elif rs == 28:
                out[i] = (gp + sx(imm)) & 0xFFFFFFFF
        if op == 0:
            hi.pop(rd, None)
        elif op == 3:
            hi.pop(31, None)
        elif 8 <= op <= 14 or 32 <= op <= 38:     # ALU-immediate and loads write rt
            hi.pop(rt, None)
    return out


def twins():
    """{(retail addr, demo addr): (demo size, retail size)}; the alignment filter drops bad pairs."""
    by_name = {f["name"]: f for f in sym["funcs"]}
    out = {}
    for l in open(P("decomp", "functions.tsv"), encoding="utf-8"):
        if l.startswith("0x"):
            a, s, n = l.split("\t")[:3]
            if n in by_name:
                out[int(a, 16), by_name[n]["addr"]] = (by_name[n]["size"], int(s))
    for m in json.load(open(P("build", "matches.json")))["exact_matches"]:
        out.setdefault((m["retail_addr"], m["demo_addr"]), (m["size"], m["size"]))
    for l in open(P("build", "fuzzy_twins.tsv")):
        if l[0] != "#":
            r, rs, _, dname, ds = l.split("\t")[:5]
            if dname in by_name:
                out.setdefault((int(r, 16), by_name[dname]["addr"]), (int(ds), int(rs)))
    return out


def demo_globals():
    """Typed globals {addr: (name, ctype, size)}, struct layouts {tag: [(off, ctype, name)]}."""
    typed, structs, cur = {}, {}, None
    for addr, tag, cls, typ, size, dims, stag, name in records(P("extern", "demo", "PROSKATE", "MAIN.SYM")):
        if cls in (10, 12):
            cur = structs.setdefault(name, []) if name not in structs else None
        elif cls in (8, 11) and cur is not None:
            cur.append((addr, ctype(typ, stag, dims), name))
        elif cls == 102:
            cur = None
        elif cls in (2, 3) and addr >= 0x80000000 and (typ >> 4) & 3 != 2:
            # the size field is 0 for scalars and arrays of them: take it from the type
            if (typ >> 4) & 3 == 1:
                size = 4
            elif not size:
                size = SCALAR.get(BASE_TYPE.get(typ & 0xF), 0)
                for n in dims:
                    size *= n
            typed.setdefault(addr, (name, ctype(typ, stag, dims), size, stag))
    return typed, structs


def member_hint(structs, stag, typ, size, off):
    """'mScale' / '[3].mScale' for an offset into a struct (or array of one)."""
    lay = structs.get(stag)
    if not lay:
        return ""
    idx = ""
    if typ.endswith("]"):
        n = int(typ[typ.rindex("[") + 1:-1] or 0)
        if n:
            idx, off = f"[{off // (size // n)}]", off % (size // n)
    best = [m for m in lay if m[0] <= off]
    return f"{idx}.{best[-1][2]}+{off - best[-1][0]:#x}" if best else idx


def c_name(name):
    """A C identifier for a demo symbol, and a fallback for when two of them collide."""
    m = re.match(r"^_\d+(\w+)\.(\w+)$", name)          # static member _7CCamera.TweakCol
    if m:
        return f"{m.group(1)}_{m.group(2)}", None
    m = re.match(r"^(\w+)\.(\d+)$", name)               # function-local static old_camDist.394
    if m:
        return m.group(1), f"{m.group(1)}_{m.group(2)}"
    return name, None


def c_string(a):
    """The C string at retail address a, or None: printable, NUL-terminated, starting a string."""
    if not GAME_CODE_END <= a < RETAIL_END:              # not tables embedded in game .text
        return None
    o = a - RETAIL_BASE + 0x800
    if a % 4 and retail[o - 1]:
        return None
    end = retail.find(b"\0", o, o + 256)
    t = retail[o:end]
    if end < 0 or len(t) < 2 or any(not (32 <= c < 127 or c in (9, 10, 13)) for c in t):
        return None
    return t.decode("latin1")


def main():
    typed, structs = demo_globals()
    funcs = (min(f["addr"] for f in sym["funcs"]), max(f["end"] for f in sym["funcs"]))
    labels = {s["addr"]: s["name"] for s in sym["symbols"]
              if not funcs[0] <= s["addr"] < funcs[1] and not SECTION.match(s["name"])}
    tkeys = sorted(typed)

    votes = collections.defaultdict(collections.Counter)
    pairs = twins()
    used = 0
    for (ra, da), (ds, rs) in pairs.items():
        dw, rw = words(demo, DEMO_BASE, da, ds), words(retail, RETAIL_BASE, ra, rs)
        sm = difflib.SequenceMatcher(None, [opkey(x) for x in dw], [opkey(x) for x in rw], autojunk=False)
        blocks = sm.get_matching_blocks()
        if sum(b.size for b in blocks) < MIN_ALIGN * min(len(dw), len(rw)):
            continue
        used += 1
        dr, rr = data_refs(dw, DEMO_GP), data_refs(rw, RETAIL_GP)
        for a, b, n in blocks:
            for k in range(n if n >= MIN_BLOCK else 0):
                if a + k in dr and b + k in rr and 0x80000000 <= rr[b + k] < 0x80200000:
                    votes[rr[b + k]][dr[a + k]] += 1

    # winner per retail address
    win = {}
    for ra, c in votes.items():
        (da, n), tot = c.most_common(1)[0], sum(c.values())
        if n >= AGREE * tot:
            win[ra] = (da, n, tot)
    # demo address -> retail address, for exact globals (to rebase offsets)
    base_retail = collections.defaultdict(collections.Counter)
    for ra, (da, n, _) in win.items():
        base_retail[da][ra] += n

    rows, ambiguous, stray = [], sum(1 for ra in votes if ra not in win), 0
    for ra, (da, n, tot) in sorted(win.items()):
        rb = base_retail[da].most_common(1)[0][0]
        if (da in typed or da in labels) and ra == rb:
            name, typ = (typed[da][0], typed[da][1]) if da in typed else (labels[da], "")
            kind, note = "exact", ""
        elif da in typed or da in labels:
            # the global's best-supported retail address is elsewhere: this is an offset into it (a
            # retail %lo folding a member offset the demo adds later), or a stray misaligned vote
            off = ra - rb
            if da not in typed or not 0 < off < typed[da][2]:
                stray += 1
                continue
            name, typ, kind = f"{typed[da][0]}_{off:x}", typed[da][1], "offset"
            hint = member_hint(structs, typed[da][3], typed[da][1], typed[da][2], off)
            note = "; ".join(x for x in (f"{typed[da][0]}{hint}" if hint else "", "demo accesses the base") if x)
        else:
            i = bisect.bisect_right(tkeys, da) - 1
            if i < 0 or da - tkeys[i] >= typed[tkeys[i]][2]:
                continue
            b = tkeys[i]
            bname, btyp, bsize, bstag = typed[b]
            off = da - b
            if base_retail[b]:
                rb = base_retail[b].most_common(1)[0][0]
                note = "" if ra - rb == off else f"retail offset {ra - rb:#x} != demo {off:#x}"
                off = ra - rb
            else:
                note = "base not referenced in retail twins"
            if off < 0:
                continue
            name, typ, kind = f"{bname}_{off:x}", btyp, "offset"
            hint = member_hint(structs, bstag, btyp, bsize, da - b)
            note = "; ".join(x for x in (f"{bname}{hint}" if hint else "", note) if x)
        rows.append([ra, name, kind, n, tot, da, typ, note])

    # layout: between two exact names the same distance apart in demo and retail, every demo
    # symbol sits at the same offset in retail
    sym_at = {a: (n, "") for a, n in labels.items()}
    sym_at.update({a: (v[0], v[1]) for a, v in typed.items()})
    skeys = sorted(sym_at)
    have = {r[0] for r in rows}
    anchors = sorted((r[0], r[5]) for r in rows if r[2] == "exact")
    for (rl, dl), (rh, dh) in zip(anchors, anchors[1:]):
        if rh - rl != dh - dl or dh <= dl:
            continue
        for da in skeys[bisect.bisect_right(skeys, dl):bisect.bisect_left(skeys, dh)]:
            if rl + da - dl not in have:
                rows.append([rl + da - dl, sym_at[da][0], "layout", 0, 0, da, sym_at[da][1],
                             f"between {sym_at[dl][0]} and {sym_at[dh][0]}"])
                have.add(rl + da - dl)

    # identifiers; colliding function-local statics fall back to their numbered names, any other
    # collision (same-named statics, two retail copies) keeps neither
    alt = {}
    for r in rows:
        r[1], alt[r[0]] = c_name(r[1])
    rows = [r for r in rows if IDENT.match(r[1])]
    count = collections.Counter(r[1] for r in rows)
    for r in rows:
        if count[r[1]] > 1 and alt[r[0]]:
            r[1] = alt[r[0]]
    count = collections.Counter(r[1] for r in rows)
    clash = sorted({r[1] for r in rows if count[r[1]] > 1})
    rows = [r for r in rows if count[r[1]] == 1]

    # hand-named globals (decomp/data_names_manual.tsv) replace whatever the passes gave
    manual = {}
    for l in open(P("decomp", "data_names_manual.tsv"), encoding="utf-8"):
        if l.startswith("0x"):
            a, n, basis = l.rstrip("\n").split("\t")[:3]
            manual[int(a, 16)] = (n, basis)
    manual_names = {n for n, _ in manual.values()}
    rows = [r for r in rows if r[0] not in manual and r[1] not in manual_names]
    rows += [[a, n, "manual", 0, 0, 0, "", basis] for a, (n, basis) in manual.items()]
    count = collections.Counter(r[1] for r in rows)

    # strings: every referenced, still unnamed address that holds a C string
    have, taken, refs = {r[0] for r in rows}, set(count), set(votes)
    for l in open(P("decomp", "functions.tsv"), encoding="utf-8"):
        if l.startswith("0x"):
            a, n = l.split("\t")[:2]
            refs.update(data_refs(words(retail, RETAIL_BASE, int(a, 16), int(n)), RETAIL_GP).values())
    for ra in sorted(refs - have):
        t = c_string(ra)
        if t is None:
            continue
        name = "s_" + (re.sub(r"[^A-Za-z0-9]+", "_", t).strip("_")[:32] or f"{ra:08x}")
        if name in taken:
            name += f"_{ra:08x}"
        taken.add(name)
        rows.append([ra, name, "string", 0, 0, 0, "char[]", repr(t)[1:-1][:60]])
    rows.sort()

    with open(P("decomp", "data_names.tsv"), "w", encoding="utf-8", newline="\n") as o:
        o.write("# Retail globals named from their THPS2-demo twins (tools/data_names.py; don't edit by hand:\n"
                "# hand names go in data_names_manual.tsv).\n"
                "# votes = aligned twin accesses agreeing / all accesses seen. kind: exact = a demo SYM global or\n"
                "# label at the twin address; offset = <Base>_<hex offset> inside a typed demo object (note: member);\n"
                "# layout = between two exact names the same distance apart in demo and retail; string = s_<text>;\n"
                "# manual = from data_names_manual.tsv (note: basis).\n"
                "# addr\tname\tkind\tvotes\tdemo_addr\tdemo_type\tnote\n")
        for ra, name, kind, n, tot, da, typ, note in rows:
            votes_s, demo_s = f"{n}/{tot}" if tot else "", f"0x{da:08x}" if da else ""
            o.write(f"0x{ra:08x}\t{name}\t{kind}\t{votes_s}\t{demo_s}\t{typ}\t{note}\n")
    print(f"twins {len(pairs)} (aligned {used}); retail data addresses seen {len(votes)}, "
          f"ambiguous {ambiguous}, stray {stray}, named {len(rows)} "
          f"({', '.join(f'{sum(r[2] == k for r in rows)} {k}' for k in ('exact', 'offset', 'layout', 'string', 'manual'))}); "
          f"name clashes dropped {len(clash)}: {' '.join(clash[:12])}{' ...' if len(clash) > 12 else ''}")


if __name__ == "__main__":
    main()
