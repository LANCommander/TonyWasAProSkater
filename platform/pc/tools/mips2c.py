#!/usr/bin/env python3
"""Translate MIPS R3000 functions from the retail executable into C (a small static recompiler).

Used for code that has no C source: Neversoft's hand-written M3DASM/M3DMATHS modules, and the pure
(non-hardware) parts of the Psy-Q libraries. The output runs in the port's address space, where PS1
RAM sits at its original addresses, so loads and stores are plain pointer accesses.

Model:
  * all general registers, HI and LO live in one global register file (`pc_r`, see pc_recomp.h);
    each function body copies it into locals on entry and back on exit/calls, so hand-written
    code that passes values to its own helpers in non-ABI registers keeps working;
  * a function is entered from native code through its wrapper `rcw_<addr>(a0..a7)`, which builds
    a PS1 stack frame below the current PS1 stack pointer (stack arguments at sp+16..);
  * calls to other translated functions call the body directly (sharing the register file); calls
    to anything else (decompiled C, HLE) are native calls through the PS1 address (a trampoline);
  * branch delay slots and the R3000 load delay slot are honoured;
  * COP2 goes to the software GTE (pc_gte.h); the BIOS jump-table stubs (`jr` to 0xa0/0xb0/0xc0)
    become PC_BiosCall.

usage: mips2c.py --exe SLUS --entries FILE --native FILE -o out.c
  entries: lines "0xADDR [name]" of functions to translate
  native:  lines "0xADDR" of addresses implemented natively (HLE/decompiled); calls there are
           native even if the address is also in entries
"""
import argparse
import struct
import sys

REG = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5",
       "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp",
       "sp", "fp", "ra"]


class Exe:
    def __init__(self, path):
        d = open(path, "rb").read()
        self.base, size = struct.unpack_from("<2I", d, 0x18)
        self.data = d[0x800:0x800 + size]
        self.end = self.base + size

    def word(self, addr):
        return struct.unpack_from("<I", self.data, addr - self.base)[0]


def s16(x):
    return x - 0x10000 if x & 0x8000 else x


class Insn:
    __slots__ = ("addr", "w", "op", "rs", "rt", "rd", "sa", "fn", "imm", "simm", "target", "kind",
                 "mnem")

    def __init__(self, addr, w):
        self.addr, self.w = addr, w
        self.op = w >> 26
        self.rs = (w >> 21) & 31
        self.rt = (w >> 16) & 31
        self.rd = (w >> 11) & 31
        self.sa = (w >> 6) & 31
        self.fn = w & 63
        self.imm = w & 0xFFFF
        self.simm = s16(self.imm)
        self.target = None
        self.kind = "op"  # op | branch | jump | call | jr | jalr | stop
        self.mnem = ""
        decode(self)

    def reads(self):
        """General registers this instruction reads (for the load-delay check)."""
        o, f = self.op, self.fn
        if o == 0:
            if f in (0, 2, 3):  # sll srl sra
                return {self.rt}
            if f in (8,):  # jr
                return {self.rs}
            if f == 9:
                return {self.rs}
            if f in (16, 18):  # mfhi mflo
                return set()
            if f in (17, 19):  # mthi mtlo
                return {self.rs}
            if f in (12, 13):
                return {2, 4, 5, 6, 7}
            return {self.rs, self.rt}
        if o == 1:
            return {self.rs}
        if o in (2, 3):
            return set()
        if o in (4, 5):
            return {self.rs, self.rt}
        if o in (6, 7):
            return {self.rs}
        if o == 15:  # lui
            return set()
        if 8 <= o <= 14:
            return {self.rs}
        if o == 18:  # cop2
            if self.rs in (4, 6):  # mtc2 ctc2
                return {self.rt}
            return set()
        if o in (32, 33, 34, 35, 36, 37, 38, 50):  # loads (lwl/lwr read rt too)
            return {self.rs} | ({self.rt} if o in (34, 38) else set())
        if o in (40, 41, 42, 43, 46, 58):  # stores
            return {self.rs, self.rt}
        return {self.rs, self.rt}

    def writes(self):
        o, f = self.op, self.fn
        if o == 0:
            if f in (8, 12, 13, 17, 19, 24, 25, 26, 27):
                return None
            return self.rd
        if o == 1:
            return 31 if self.rt in (16, 17) else None
        if o == 3:
            return 31
        if 8 <= o <= 15:
            return self.rt
        if o == 18 and self.rs in (0, 2):  # mfc2 cfc2
            return self.rt
        if o in (32, 33, 34, 35, 36, 37, 38):
            return self.rt
        return None

    def is_load(self):
        return self.op in (32, 33, 34, 35, 36, 37, 38) or (self.op == 18 and self.rs in (0, 2))


def decode(i):
    o, f = i.op, i.fn
    if o == 0:
        if f == 8:
            i.kind = "jr"
        elif f == 9:
            i.kind = "jalr"
        elif f == 12:
            i.kind = "syscall"
        elif f == 13:
            i.kind = "break"
    elif o == 1:
        i.kind = "branch"
        i.target = (i.addr + 4 + (i.simm << 2)) & 0xFFFFFFFF
        if i.rt in (16, 17):
            i.kind = "branchcall"
    elif o == 2:
        i.kind = "jump"
        i.target = ((i.addr + 4) & 0xF0000000) | ((i.w & 0x3FFFFFF) << 2)
    elif o == 3:
        i.kind = "call"
        i.target = ((i.addr + 4) & 0xF0000000) | ((i.w & 0x3FFFFFF) << 2)
    elif o in (4, 5, 6, 7):
        i.kind = "branch"
        i.target = (i.addr + 4 + (i.simm << 2)) & 0xFFFFFFFF


def R(n):
    return "0" if n == 0 else REG[n]


def mem(i):
    base = R(i.rs)
    return "(%s + %d)" % (base, i.simm) if i.simm else base


def op_c(i, dst_override=None):
    """C statement(s) for a non-control instruction. dst_override replaces the written register
    name (used for the load-delay temp)."""
    o, f = i.op, i.fn
    rs, rt, rd = R(i.rs), R(i.rt), R(i.rd)

    def W(reg, expr):
        if reg == 0 and dst_override is None:
            return ""
        return "%s = %s;" % (dst_override or REG[reg], expr)

    if o == 0:
        if f == 0:
            return W(i.rd, "%s << %d" % (rt, i.sa)) if i.w else ""
        if f == 2:
            return W(i.rd, "%s >> %d" % (rt, i.sa))
        if f == 3:
            return W(i.rd, "(u32)((s32)%s >> %d)" % (rt, i.sa))
        if f == 4:
            return W(i.rd, "%s << (%s & 31)" % (rt, rs))
        if f == 6:
            return W(i.rd, "%s >> (%s & 31)" % (rt, rs))
        if f == 7:
            return W(i.rd, "(u32)((s32)%s >> (%s & 31))" % (rt, rs))
        if f == 16:
            return W(i.rd, "hi")
        if f == 17:
            return "hi = %s;" % rs
        if f == 18:
            return W(i.rd, "lo")
        if f == 19:
            return "lo = %s;" % rs
        if f == 24:
            return "{ s64 p_ = (s64)(s32)%s * (s32)%s; lo = (u32)p_; hi = (u32)(p_ >> 32); }" % (rs, rt)
        if f == 25:
            return "{ u64 p_ = (u64)%s * %s; lo = (u32)p_; hi = (u32)(p_ >> 32); }" % (rs, rt)
        if f == 26:
            return "rc_div(&lo, &hi, %s, %s);" % (rs, rt)
        if f == 27:
            return "rc_divu(&lo, &hi, %s, %s);" % (rs, rt)
        if f in (32, 33):
            return W(i.rd, "%s + %s" % (rs, rt))
        if f in (34, 35):
            return W(i.rd, "%s - %s" % (rs, rt))
        if f == 36:
            return W(i.rd, "%s & %s" % (rs, rt))
        if f == 37:
            return W(i.rd, "%s | %s" % (rs, rt))
        if f == 38:
            return W(i.rd, "%s ^ %s" % (rs, rt))
        if f == 39:
            return W(i.rd, "~(%s | %s)" % (rs, rt))
        if f == 42:
            return W(i.rd, "(s32)%s < (s32)%s" % (rs, rt))
        if f == 43:
            return W(i.rd, "%s < %s" % (rs, rt))
        raise ValueError("unknown SPECIAL fn %d at %08x" % (f, i.addr))
    if o in (8, 9):
        return W(i.rt, "%s + %d" % (rs, i.simm))
    if o == 10:
        return W(i.rt, "(s32)%s < %d" % (rs, i.simm))
    if o == 11:
        return W(i.rt, "%s < 0x%xu" % (rs, i.simm & 0xFFFFFFFF))
    if o == 12:
        return W(i.rt, "%s & 0x%x" % (rs, i.imm))
    if o == 13:
        return W(i.rt, "%s | 0x%x" % (rs, i.imm))
    if o == 14:
        return W(i.rt, "%s ^ 0x%x" % (rs, i.imm))
    if o == 15:
        return W(i.rt, "0x%xu" % (i.imm << 16))
    if o == 32:
        return W(i.rt, "(u32)(s32)MEM8S(%s)" % mem(i))
    if o == 33:
        return W(i.rt, "(u32)(s32)MEM16S(%s)" % mem(i))
    if o == 35:
        return W(i.rt, "MEM32(%s)" % mem(i))
    if o == 36:
        return W(i.rt, "MEM8(%s)" % mem(i))
    if o == 37:
        return W(i.rt, "MEM16(%s)" % mem(i))
    if o == 34:
        return W(i.rt, "rc_lwl(%s, %s)" % (rt, mem(i)))
    if o == 38:
        return W(i.rt, "rc_lwr(%s, %s)" % (rt, mem(i)))
    if o == 40:
        return "MEM8(%s) = (u8)%s;" % (mem(i), rt)
    if o == 41:
        return "MEM16(%s) = (u16)%s;" % (mem(i), rt)
    if o == 43:
        return "MEM32(%s) = %s;" % (mem(i), rt)
    if o == 42:
        return "rc_swl(%s, %s);" % (rt, mem(i))
    if o == 46:
        return "rc_swr(%s, %s);" % (rt, mem(i))
    if o == 50:  # lwc2
        return "PC_GteSetData(%d, MEM32(%s));" % (i.rt, mem(i))
    if o == 58:  # swc2
        return "MEM32(%s) = PC_GteGetData(%d);" % (mem(i), i.rt)
    if o == 18:
        if i.rs & 0x10:
            return "PC_GteCommand(0x%07x);" % (i.w & 0x1FFFFFF)
        if i.rs == 0:
            return W(i.rt, "PC_GteGetData(%d)" % i.rd)
        if i.rs == 2:
            return W(i.rt, "PC_GteGetCtrl(%d)" % i.rd)
        if i.rs == 4:
            return "PC_GteSetData(%d, %s);" % (i.rd, rt)
        if i.rs == 6:
            return "PC_GteSetCtrl(%d, %s);" % (i.rd, rt)
    if o == 16:  # cop0: interrupts/status; nothing to do natively
        if i.rs == 0:
            return W(i.rt, "rc_cop0_read(%d)" % i.rd)
        return ""
    raise ValueError("unknown opcode %d at %08x (%08x)" % (o, i.addr, i.w))


def branch_cond(i):
    rs, rt = R(i.rs), R(i.rt)
    o = i.op
    if o == 4:
        return "%s == %s" % (rs, rt)
    if o == 5:
        return "%s != %s" % (rs, rt)
    if o == 6:
        return "(s32)%s <= 0" % rs
    if o == 7:
        return "(s32)%s > 0" % rs
    if o == 1:
        if i.rt in (0, 16):
            return "(s32)%s < 0" % rs
        if i.rt in (1, 17):
            return "(s32)%s >= 0" % rs
    raise ValueError("bad branch at %08x" % i.addr)


class Func:
    def __init__(self, entry, name):
        self.entry, self.name = entry, name
        self.insns = {}
        self.labels = set()
        self.local_calls = set()  # return addresses of jal into own code
        self.max_addr = entry


def discover(exe, entry, known_entries):
    """Collect the instructions reachable from entry without crossing into another function."""
    f = Func(entry, None)
    work = [entry]
    seen = set()
    while work:
        pc = work.pop()
        while pc not in seen:
            if not (exe.base <= pc < exe.end):
                break
            seen.add(pc)
            i = Insn(pc, exe.word(pc))
            f.insns[pc] = i
            if i.kind in ("branch", "branchcall"):
                ds = pc + 4
                if ds not in seen:
                    seen.add(ds)
                    f.insns[ds] = Insn(ds, exe.word(ds))
                if i.kind == "branch":
                    f.labels.add(i.target)
                    work.append(i.target)
                pc += 8
                continue
            if i.kind == "jump":
                ds = pc + 4
                seen.add(ds)
                f.insns[ds] = Insn(ds, exe.word(ds))
                if i.target in known_entries and i.target != entry:
                    break  # tail call
                f.labels.add(i.target)
                work.append(i.target)
                break
            if i.kind in ("call", "jalr"):
                ds = pc + 4
                seen.add(ds)
                f.insns[ds] = Insn(ds, exe.word(ds))
                pc += 8
                continue
            if i.kind == "jr":
                ds = pc + 4
                seen.add(ds)
                f.insns[ds] = Insn(ds, exe.word(ds))
                break
            if i.kind == "break" and i.w == 0x0000004D:
                pc += 4
                continue
            pc += 4
    # local subroutine calls: jal to an address inside our own code that isn't a known entry
    for a, i in list(f.insns.items()):
        if i.kind == "call" and i.target not in known_entries and i.target in f.insns:
            f.labels.add(i.target)
            f.local_calls.add(a + 8)
            f.labels.add(a + 8)
    f.max_addr = max(f.insns)
    return f


def is_bios_stub(f):
    """Psy-Q BIOS stubs: `addiu t2,zero,0xa0; jr t2; addiu t1,zero,n` (or 0xb0/0xc0)."""
    ins = [f.insns[a] for a in sorted(f.insns)]
    if len(ins) != 3:
        return None
    a, b, c = ins
    if a.op == 9 and a.rs == 0 and a.rt == 10 and a.imm in (0xA0, 0xB0, 0xC0) and \
            b.kind == "jr" and b.rs == 10 and c.op == 9 and c.rs == 0 and c.rt == 9:
        return a.imm, c.imm
    return None


def emit_function(f, translated, native, out):
    ent = f.entry
    bios = is_bios_stub(f)
    out.append("/* %s */" % (f.name or "0x%08x" % ent))
    out.append("void rb_%08x(void) {" % ent)
    if bios:
        out.append("\tpc_r.r[2] = PC_BiosCall(0x%x, 0x%x);" % bios)
        out.append("}")
        return
    out.append("\tREGS_DECL")
    out.append("\tREGS_IN();")
    addrs = sorted(f.insns)
    # targets needing a label: branch/jump targets and anything after a delay slot jumped into
    labels = set(f.labels) | {ent}
    has_indirect = any(f.insns[a].kind == "jr" and f.insns[a].rs != 31 for a in addrs)
    if has_indirect:
        labels |= set(addrs)
    skip = set()
    k = 0
    while k < len(addrs):
        a = addrs[k]
        i = f.insns[a]
        if a in skip:
            k += 1
            continue
        if a in labels:
            out.append("L_%08x:" % a)
        if i.kind in ("branch", "branchcall", "jump", "call", "jr", "jalr"):
            ds = f.insns.get(a + 4)
            dsc = op_c(ds) if ds else ""
            if i.kind == "branch":
                out.append("\t{ int c_ = %s; %s if (c_) goto L_%08x; }" % (branch_cond(i), dsc,
                                                                         i.target))
            elif i.kind == "branchcall":
                # bltzal/bgezal: rare; treat as conditional call
                out.append("\t{ int c_ = %s; ra = 0x%08xu; %s if (c_) { %s } }" % (
                    branch_cond(i), a + 8, dsc, call_c(i.target, a + 8, f, translated, native)))
            elif i.kind == "jump":
                if i.target in f.insns:
                    out.append("\t%s goto L_%08x;" % (dsc, i.target))
                else:
                    # tail call: the target returns to our caller
                    out.append("\t%s %s goto ret_;" % (dsc, call_c(i.target, None, f, translated,
                                                                   native, tail=True)))
            elif i.kind == "call":
                if i.target in f.insns and i.target not in translated:
                    out.append("\tra = 0x%08xu; %s goto L_%08x;" % (a + 8, dsc, i.target))
                else:
                    out.append("\tra = 0x%08xu; %s %s" % (a + 8, dsc, call_c(i.target, a + 8, f,
                                                                          translated, native)))
            elif i.kind == "jalr":
                rd = REG[i.rd] if i.rd else "ra"
                out.append("\t{ u32 t_ = %s; %s = 0x%08xu; %s rc_call_indirect(t_); REGS_IN(); }" % (
                    R(i.rs), rd, a + 8, (dsc + " REGS_OUT();") if dsc else "REGS_OUT();"))
            elif i.kind == "jr":
                if i.rs == 31:
                    if f.local_calls:
                        cases = " ".join("case 0x%08xu: goto L_%08x;" % (r, r)
                                         for r in sorted(f.local_calls))
                        out.append("\t{ u32 t_ = ra; %s switch (t_) { %s default: goto ret_; } }" % (
                            dsc, cases))
                    else:
                        out.append("\t%s goto ret_;" % dsc)
                else:
                    cases = " ".join("case 0x%08xu: goto L_%08x;" % (x, x) for x in addrs)
                    out.append("\t{ u32 t_ = %s; %s switch (t_) { %s default: "
                               "rc_bad_jump(0x%08xu, t_); goto ret_; } }" % (R(i.rs), dsc, cases, a))
            # the delay slot: emitted inline above; give it its own copy if something jumps to it
            if ds:
                if (a + 4) in labels:
                    out.append("\tgoto L_%08x_n;" % (a + 4))
                    out.append("L_%08x:" % (a + 4))
                    out.append("\t%s" % dsc)
                    out.append("L_%08x_n:;" % (a + 4))
                skip.add(a + 4)
            k += 1
            continue
        if i.kind == "syscall":
            out.append("\tREGS_OUT(); PC_Syscall(); REGS_IN();")
            k += 1
            continue
        if i.kind == "break":
            out.append("\trc_break(0x%08xu);" % a)
            k += 1
            continue
        # R3000 load delay: the next instruction still sees the old value
        nxt = f.insns.get(a + 4)
        w = i.writes() if i.is_load() else None
        if w and nxt is not None and (a + 4) not in labels and w in nxt.reads() and \
                nxt.kind == "op" and not nxt.is_load():
            # (a delay-slot instruction that also writes the register cancels the load)
            tail = "" if nxt.writes() == w else "%s = ld_;" % REG[w]
            out.append("\t{ u32 ld_; %s %s %s }" % (op_c(i, "ld_"), op_c(nxt), tail))
            skip.add(a + 4)
            k += 1
            continue
        s = op_c(i)
        if s:
            out.append("\t" + s)
        k += 1
    # falling off the end (shouldn't happen) returns
    out.append("ret_:")
    out.append("\tREGS_OUT();")
    out.append("}")


def call_c(target, ret, f, translated, native, tail=False):
    if target in translated and target not in native:
        return "REGS_OUT(); rb_%08x(); REGS_IN();" % target
    return "REGS_OUT(); rc_call_native(0x%08xu); REGS_IN();" % target


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--entries", required=True)
    ap.add_argument("--native", default=None)
    ap.add_argument("--known", default=None, help="extra known function entries (one per line)")
    ap.add_argument("-o", "--out", required=True)
    a = ap.parse_args()
    exe = Exe(a.exe)
    entries = {}
    for line in open(a.entries):
        p = line.split()
        if p and not p[0].startswith("#"):
            entries[int(p[0], 16)] = p[1] if len(p) > 1 else None
    native = set()
    if a.native:
        for line in open(a.native):
            p = line.split()
            if p and not p[0].startswith("#"):
                native.add(int(p[0], 16))
    known = set(entries) | native
    if a.known:
        for line in open(a.known):
            p = line.split()
            if p and not p[0].startswith("#"):
                known.add(int(p[0], 16))
    funcs = []
    for e in sorted(entries):
        if e in native:
            continue
        f = discover(exe, e, known)
        f.name = entries[e]
        funcs.append(f)
    translated = {f.entry for f in funcs}
    out = ['/* Generated by platform/pc/tools/mips2c.py from the retail executable. Do not edit. */',
           '#include "pc_recomp.h"', ""]
    for f in funcs:
        out.append("void rb_%08x(void);" % f.entry)
    out.append("")
    for f in funcs:
        try:
            body = []
            emit_function(f, translated, native, body)
            out.extend(body)
        except ValueError as e:
            print("mips2c: %s: %s" % (f.name or hex(f.entry), e), file=sys.stderr)
            out.append("void rb_%08x(void) { rc_untranslatable(0x%08xu); }" % (f.entry, f.entry))
        out.append("")
    # native entry wrappers + table
    for f in funcs:
        out.append("u32 rcw_%08x(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7) "
                   "{ return rc_enter(rb_%08x, a0, a1, a2, a3, a4, a5, a6, a7); }" % (f.entry, f.entry))
    out.append("const PcFunc pc_recomp_funcs[] = {")
    for f in funcs:
        out.append("\t{0x%08xu, (void *)rcw_%08x}," % (f.entry, f.entry))
    out.append("\t{0, 0}\n};")
    out.append("const PcRecompBody pc_recomp_bodies[] = {")
    for f in funcs:
        out.append("\t{0x%08xu, rb_%08x}," % (f.entry, f.entry))
    out.append("\t{0, 0}\n};")
    open(a.out, "w").write("\n".join(out) + "\n")
    print("mips2c: %d functions -> %s" % (len(funcs), a.out))


if __name__ == "__main__":
    main()
