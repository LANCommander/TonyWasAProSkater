#!/usr/bin/env python3
"""
Matcher for DIVISION functions, using the real ASPSX assembler (psyq-asm can't emit bare div).
Pipeline:  cc1psx -> rewrite `div $d,$s,$t` macro to bare `div $s,$t` + `mflo $d` -> ASPSX.EXE ->
DMPSX (GTE sources) -> parse the LNK object, resolve its patches -> byte-diff vs SLUS_008.60.

  python tools/match_div.py <src.c> <retailAddrHex> <sizeBytes> [syms.json] [-O2] [-G0|-G8]

syms.json is the same map match_sym.mjs takes ({"gp":..,"<name>":"0x.."}); the function's own .text
is placed at the target address, so local jumps resolve too. Assumes the target is the first
function in the file.
"""
import json
import re
import struct
import subprocess
import sys
import os

CC1 = os.environ.get("CC1", r"D:\Tools\psyq4.5\CC1PSX.EXE")
CC1PL = os.environ.get("CC1PL", r"D:\Tools\psyq4.5\CC1PLPSX.EXE")  # C++ front end, used for .cpp sources
ASPSX = r"D:\Tools\psyq4.5\ASPSX.EXE"
CPP = r"D:\Tools\psyq4.5\CPPPSX.EXE"
DMPSX = r"D:\Tools\psyq4.5\DMPSX.EXE"
INC = os.environ.get("PSYQ_INC", r"D:\Tools\psyq4.5\include")
# the repo's own headers (include/names.h: readable aliases for g++-mangled names)
LOCAL_INC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "include")
# what CCPSX hands cpp for C on the PSX target (mirrors tools/psyq.mjs)
CPP_DEFS = ["-undef", "-D__GNUC__=2", "-D__OPTIMIZE__", "-Dmips", "-D__mips__", "-D__psx__", "-D_LANGUAGE_C"]
RETAIL = "../extern/thps1_retail/SLUS_008.60"
TEXT_ADDR = 0x80010000
HDR = 0x800

src, addrHex, sizeStr = sys.argv[1], sys.argv[2], sys.argv[3]
addr, size = int(addrHex, 16), int(sizeStr)
symPath = next((a for a in sys.argv[4:] if a.endswith(".json")), None)
syms = json.load(open(symPath)) if symPath else {}
opt = next((a for a in sys.argv[4:] if a.startswith("-O")), "-O2")
g = "0" if "-G0" in sys.argv else ("8" if "-G8" in sys.argv else "0")

base = os.path.join("build", f"div_{os.getpid()}")
os.makedirs("build", exist_ok=True)
sfile, rfile, ofile = base + ".s", base + "_rw.s", base + ".obj"

# cc1 has no preprocessor (not even comment stripping), so always run CPPPSX first, as CCPSX does
csrc = base + ".i"
subprocess.run([CPP, *CPP_DEFS, "-I", LOCAL_INC, "-I", INC, src, csrc], check=True, capture_output=True)
subprocess.run([CC1PL if src.endswith(".cpp") and ("CC1PL" in os.environ or "CC1" not in os.environ) else CC1, csrc, "-quiet", *opt.split(), *(["-fno-exceptions"] if src.endswith(".cpp") and "NOEXC" not in os.environ else []), *os.environ.get("CC1_OPTS", "").split(), f"-G{g}", "-o", sfile], check=True)
s = open(sfile).read()
# STRIPDBG=1 (with CC1_OPTS=-g): drop the SDB debug directives, whose LNK records the parser can't read
if os.environ.get("STRIPDBG"):
    s = "\n".join(l for l in s.split("\n") if not re.match(r"\s*\.(def|loc|bgnb|endb|bend|begin|file)\b", l))
# div/divu/rem/remu macro -> bare op + mflo/mfhi, i.e. ASPSX's macro expansion minus the
# divide-by-zero/overflow checks. ASPSX treats a macro-expanded mflo/mfhi like a load: when the next
# instruction reads the result it inserts a nop, which it doesn't for a bare mflo, so emit it here.
DIV = re.compile(r'^\t(div|divu|rem|remu)\t(\$\w+),(\$\w+),(\$\w+)\s*$')
INSN = re.compile(r'^\t([a-z][\w.]*)\t?(.*)$')


def reads(line, reg):
    m = INSN.match(line)
    if not m: return False
    ops = [o.strip() for o in m.group(2).split("#")[0].split(",") if o.strip()]
    regs = lambda o: re.findall(r"\$\w+", o)
    # stores/branches read every operand; otherwise operand 0 is the destination (but a memory
    # operand's base register is still read)
    # (mtc2/ctc2 read their first operand too: `mtc2 $2,$8` moves $2 into GTE register 8)
    if m.group(1) in ("sw", "sh", "sb", "swl", "swr", "jr", "jalr", "mtc2", "ctc2") or m.group(1).startswith("b"):
        return any(reg in regs(o) for o in ops)
    return any(reg in regs(o) for o in ops[1:]) or bool(ops and "(" in ops[0] and reg in regs(ops[0]))


lines, out = s.split("\n"), []
for k, line in enumerate(lines):
    m = DIV.match(line)
    if not m: out.append(line); continue
    op, d, x, y = m.groups()
    out += [f"\t{'div' if op == 'rem' else 'divu' if op == 'remu' else op}\t{x},{y}",
            f"\t{'mfhi' if op.startswith('rem') else 'mflo'}\t{d}"]
    # look past `.set noreorder` etc.: ASPSX still adds the nop when a directive sits in between
    nxt = next((l for l in lines[k + 1:] if l.strip() and not l.lstrip().startswith(("#", ".set"))), "")
    if reads(nxt, d): out.append("\tnop")
s = "\n".join(out)
open(rfile, "w").write(s)
subprocess.run([ASPSX, rfile, "-o", ofile], check=True, capture_output=True)
# INLINE_C.H GTE commands are `.word (n<<6)|0x3f` placeholders; DMPSX patches them into cop2 in place
if re.search(r"\.word\s+0x0000[0-9a-fA-F]{4}", s):
    subprocess.run([DMPSX, ofile, "-b"], check=True, capture_output=True)

tb = open(RETAIL, "rb").read()[addr - TEXT_ADDR + HDR:][:size]
want = list(struct.unpack_from(f"<{size//4}I", tb, 0))


def read_lnk(path):
    """Parse a Psy-Q LNK object: per-section code bytes, patches, and symbol id -> name/location."""
    b = open(path, "rb").read()
    assert b[:3] == b"LNK", "not a Psy-Q LNK object"
    i = 4
    u8 = lambda: b[i]
    code, sect_names, names, local, patches = {}, {}, {}, {}, []
    cur = None
    rec_start = {}   # patch offsets are relative to the section's most recent code record

    def take(fmt):
        nonlocal i
        v = struct.unpack_from("<" + fmt, b, i); i += struct.calcsize("<" + fmt); return v

    def pstr():
        nonlocal i
        n = b[i]; i += 1; v = b[i:i + n].decode("latin-1"); i += n; return v

    def expr():
        (op,) = take("B")
        if op == 0x00: return ("const", take("I")[0])
        if op == 0x02: return ("sym", take("H")[0])
        if op == 0x04: return ("sect", take("H")[0])
        if op in (0x2C, 0x2E): return ("+" if op == 0x2C else "-", expr(), expr())
        raise ValueError(f"unknown LNK expression op 0x{op:02x} at {i - 1:#x}")

    while True:
        (rec,) = take("B")
        if rec == 0x00: break
        elif rec == 0x02:
            (n,) = take("H"); c = code.setdefault(cur, bytearray()); rec_start[cur] = len(c)
            c.extend(b[i:i + n]); i += n
        elif rec == 0x06: (cur,) = take("H")
        elif rec == 0x08: take("I")                                          # uninitialised space
        elif rec == 0x0A:
            t, off = take("BH"); patches.append((cur, t, rec_start.get(cur, 0) + off, expr()))
        elif rec == 0x0C:
            sid, sec, off = take("HHI"); names[sid] = pstr(); local[sid] = (sec, off)
        elif rec == 0x0E: (sid,) = take("H"); names[sid] = pstr()
        elif rec == 0x10: sid, _, _ = take("HHB"); sect_names[sid] = pstr()
        elif rec == 0x12: take("HI"); pstr()                                 # local symbol
        elif rec == 0x1C: take("H"); pstr()                                  # source file name
        elif rec == 0x2E: take("B")                                          # processor type
        elif rec == 0x30: sid, _, _ = take("HHI"); names[sid] = pstr()       # XBSS (common)
        elif rec == 0x3C: take("H")
        elif rec == 0x4A: take("HIHIHIHIi"); pstr()                          # function start
        elif rec in (0x4C, 0x4E, 0x50): take("HII")                          # function end, block start/end
        else: raise ValueError(f"unknown LNK record 0x{rec:02x} at {i - 1:#x}")
    return code, sect_names, names, local, patches


code, sect_names, names, local, patches = read_lnk(ofile)
text = next(k for k, v in sect_names.items() if v == ".text")
buf = code.get(text, bytearray())
size_ok = len(buf) == size
if not size_ok:   # still diff it (zero-padded/truncated) so the misalignment is visible
    print(f"note: object .text is {len(buf)} bytes, target is {size}")
    buf = buf[:size] + bytearray(max(0, size - len(buf)))

# Linker-lite, as in match_sym.mjs: .text sits at the target address, externs come from syms.json.
gp = int(syms.get("gp", "0"), 16)


def sym_addr(sid):
    n = names.get(sid)
    if n in syms: return int(syms[n], 16)
    if sid in local and local[sid][0] == text: return addr + local[sid][1]
    raise KeyError(f"unknown symbol {n!r} (add it to syms.json)")


# Other sections (a switch's .rdata jump table) sit at their retail address: from syms.json
# (".rdata": "0x…") or inferred from retail's own HI16/LO16 pair that addresses the section (or
# piece by piece, below).
retail_img = open(RETAIL, "rb").read()
rword = lambda a: struct.unpack_from("<I", retail_img, a - TEXT_ADDR + HDR)[0]
base = {text: addr}
for sid, nm in sect_names.items():
    if nm in syms: base[sid] = int(syms[nm], 16)


def sect_ref(e):
    """(section id, offset) if e is `sect` or `sect + const`, else None."""
    if e[0] == "sect": return e[1], 0
    if e[0] == "+" and e[1][0] == "sect" and e[2][0] == "const": return e[1][1], e[2][1]
    return None


for sec, t, off, e in patches:
    ref = sect_ref(e)
    if sec != text or t != 82 or not ref or ref[0] in base: continue
    lo = next((o for s2, t2, o, e2 in patches if s2 == text and t2 == 84 and e2 == e), None)
    if lo is None: continue
    hi, l = rword(addr + off) & 0xFFFF, rword(addr + lo) & 0xFFFF
    base[ref[0]] = ((hi << 16) + (l - 0x10000 if l & 0x8000 else l) - ref[1]) & 0xFFFFFFFF


def via_symbol(sec, off):
    """Address of `sec + off` through a global the source defines there (C++ puts defined globals in
    .sdata/.data instead of common), so several defined globals each get their own syms.json address."""
    best = None
    for sid, (s2, o2) in local.items():
        if s2 == sec and o2 <= off and names.get(sid) in syms and (best is None or o2 > best[1]): best = (sid, o2)
    return None if best is None else int(syms[names[best[0]]], 16) + off - best[1]


pieces = {}  # section id -> {offset: retail address}, when placed piece by piece (below)


def ev(e):
    if e[0] == "const": return e[1]
    ref = sect_ref(e)
    if ref and ref[1] in pieces.get(ref[0], {}): return pieces[ref[0]][ref[1]]
    if ref and (ref[0] in pieces or ref[0] not in base) and via_symbol(*ref) is not None: return via_symbol(*ref)
    if e[0] == "sym": return sym_addr(e[1])
    if e[0] == "sect":
        if e[1] in base: return base[e[1]]
        raise KeyError(f"no address for section {sect_names.get(e[1])} (add it to syms.json)")
    return ev(e[1]) + ev(e[2]) if e[0] == "+" else ev(e[1]) - ev(e[2])


def patch(sec, data, limit):
    for s2, t, off, e in patches:
        if s2 != sec or off >= limit: continue
        v, (w,) = ev(e) & 0xFFFFFFFF, struct.unpack_from("<I", data, off)
        if t == 16:    w = v                                                       # WORD32 (jump-table entry)
        elif t == 74:  w = (w & 0xFC000000) | ((v >> 2) & 0x03FFFFFF)             # j/jal target
        elif t == 82:  w = (w & 0xFFFF0000) | (((v + 0x8000) >> 16) & 0xFFFF)      # HI16 (paired carry)
        elif t == 84:  w = (w & 0xFFFF0000) | (v & 0xFFFF)                         # LO16
        elif t == 100: w = (w & 0xFFFF0000) | ((v - gp) & 0xFFFF)                  # GPREL16
        else: raise ValueError(f"unhandled LNK patch type {t} at {sect_names.get(sec)}+{off:#x}")
        struct.pack_into("<I", data, off, w)


# A data section the code references at several places can be placed piece by piece, as in
# psyq.mjs link: each reference resolves to the address retail's own instruction encodes there, if
# retail has the same bytes at that address (each piece, from its reference to the next one,
# checked on its own), so a one-function source's string literals match wherever retail's
# whole-file TU put them.
def simm(v): return v - 0x10000 if v & 0x8000 else v


refs = {}
for sec, t, off, e in patches:
    ref = sect_ref(e)
    if sec != text or not ref or ref[0] == text: continue
    if t == 100: a = (gp + simm(rword(addr + off) & 0xFFFF)) & 0xFFFFFFFF
    elif t in (82, 84):
        pair = lambda k: off if t == k else next((o for s2, t2, o, e2 in patches if s2 == text and t2 == k and e2 == e), None)
        hi, lo = pair(82), pair(84)
        if hi is None or lo is None: continue
        a = (((rword(addr + hi) & 0xFFFF) << 16) + simm(rword(addr + lo) & 0xFFFF)) & 0xFFFFFFFF
    else: continue
    m = refs.setdefault(ref[0], {})
    m[ref[1]] = a if m.get(ref[1], a) == a else None
for sid, m in refs.items():
    if None in m.values() or not code.get(sid): continue
    b = base.get(sid)
    if b is not None and all(a == b + o for o, a in m.items()): continue   # one base does it
    # a global the source defines there is a piece of its own, at its syms.json address (its
    # value isn't checked, as without pieces)
    globs = {o2: int(syms[names[s2id]], 16) for s2id, (s2, o2) in local.items()
             if s2 == sid and names.get(s2id) in syms}
    if any(o in m and m[o] != a for o, a in globs.items()): continue
    d = bytearray(code[sid]); patch(sid, d, len(d))
    bounds = sorted(set(m) | set(globs))
    ok = True
    for k, o in enumerate(bounds):
        if o not in m or o in globs: continue
        piece = d[o:bounds[k + 1] if k + 1 < len(bounds) else len(d)]
        n = len(piece.rstrip(bytes(1)))
        need = n + 1 if n < len(piece) else n      # trailing padding isn't compared, a string's NUL is
        a = m[o]
        at = a - TEXT_ADDR + HDR
        if not (TEXT_ADDR <= a and a + need <= TEXT_ADDR + len(retail_img) - HDR) or \
                retail_img[at:at + need] != bytes(piece[:need]):
            ok = False
            break
    if ok: pieces[sid] = {**m, **globs}

patch(text, buf, size)
got = list(struct.unpack_from(f"<{size//4}I", buf, 0))

# byte-check placed data sections (jump tables) against retail too
data_ok = True
for sid, m in pieces.items():
    print(f"{sect_names[sid]}: {len(m)} pieces (string literals...) at the retail addresses the code names, bytes match")
for sid, b in base.items():
    if sid == text or not code.get(sid) or sid in pieces: continue
    if not (TEXT_ADDR <= b and b + len(code[sid]) <= TEXT_ADDR + len(retail_img) - HDR):
        print(f"{sect_names[sid]}: not placed in the retail image (compiler-emitted data such as a vtable), not checked")
        continue
    d = code[sid]; patch(sid, d, len(d))
    n = len(d) // 4
    ok = sum(struct.unpack_from("<I", d, 4 * i)[0] == rword(b + 4 * i) for i in range(n))
    print(f"{sect_names[sid]} @{b:#x}: {ok}/{n} words match retail")
    data_ok &= ok == n

same = sum(1 for a, b in zip(got, want) if a == b)
if same != len(want) or not size_ok:
    # alignment-aware score (LCS of words): robust to one inserted/missing word shifting the rest
    patch(text, code[text], len(code[text]))   # the untruncated object (idempotent if already patched)
    full =list(struct.unpack_from(f"<{len(code[text]) // 4}I", code[text], 0))
    prev = [0] * (len(want) + 1)
    for x in full:
        cur = [0]
        for j, y in enumerate(want): cur.append(prev[j] + 1 if x == y else max(prev[j + 1], cur[j]))
        prev = cur
    print(f"aligned {prev[-1]}/{len(want)} ({len(full)} words compiled)")
if os.environ.get("D") and (same != len(want) or not size_ok):
    # D=1: disassembled, LCS-aligned hunks only (tools/wdiff.mjs), like match_sym.mjs D=1
    wfile = sfile[:-2] + "_words.json"
    json.dump({"mine": full, "retail": want}, open(wfile, "w"))
    sys.stdout.flush()
    subprocess.run(["node", os.path.join(os.path.dirname(__file__), "wdiff.mjs"), wfile])
elif same != len(want) or "-v" in sys.argv:
    for i in range(len(want)):
        mark = "" if got[i] == want[i] else " X"
        print(f"{i:2} {got[i]:08x} | {want[i]:08x}{mark}")
print(f"{same}/{len(want)}" + ("  >>> EXACT <<<" if same == len(want) and data_ok and size_ok else ""))
