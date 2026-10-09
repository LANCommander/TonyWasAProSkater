/* R3000 reference interpreter for differential tests (see mips_ref.h). */
#include "mips_ref.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RETURN_ADDR 0xfffffff0u /* ra sentinel: the call is over when pc gets here */
#define MAX_STEPS 10000000

static void Die(MRef *m, const char *why, uint32_t word) {
    fprintf(stderr, "mips_ref: %s (pc=%08x word=%08x)\n", why, m->pc, word);
    exit(2);
}

static jmp_buf *trap; /* set while MRef_Call runs */

/* What the PS1 would answer with a bus error or an exception: ends the call with m->fault. */
static void Fault(MRef *m) {
    m->fault = 1;
    if (trap)
        longjmp(*trap, 1);
    fprintf(stderr, "mips_ref: fault outside a call (pc=%08x)\n", m->pc);
    exit(2);
}

uint8_t *MRef_Ptr(MRef *m, uint32_t addr) {
    uint32_t phys = addr & 0x1fffffffu;
    if (phys < MREF_RAM_SIZE)
        return m->ram + phys;
    if (phys - 0x1f800000u < MREF_SCRATCH_SIZE)
        return m->scratch + (phys - 0x1f800000u);
    Fault(m);
    return NULL;
}

/* GTE LZCR: leading bits equal to LZCS's sign bit. */
static uint32_t Lzcr(uint32_t x) {
    uint32_t n = 0;
    if (x & 0x80000000u)
        x = ~x;
    while (n < 32 && !(x & 0x80000000u)) {
        x <<= 1;
        n++;
    }
    return n;
}

int MRef_Load(MRef *m, const char *exe_path) {
    uint8_t hdr[0x800];
    uint32_t base, size;
    FILE *f = fopen(exe_path, "rb");
    memset(m, 0, sizeof *m);
    if (!f)
        return 0;
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr || memcmp(hdr, "PS-X EXE", 8)) {
        fclose(f);
        return 0;
    }
    memcpy(&base, hdr + 0x18, 4);
    memcpy(&size, hdr + 0x1c, 4);
    m->ram = calloc(1, MREF_RAM_SIZE);
    if ((base & 0x1fffffffu) + size > MREF_RAM_SIZE ||
        fread(m->ram + (base & 0x1fffffffu), 1, size, f) != size) {
        fclose(f);
        MRef_Free(m);
        return 0;
    }
    fclose(f);
    return 1;
}

void MRef_Free(MRef *m) {
    free(m->ram);
    m->ram = NULL;
}

static uint32_t Rd32(MRef *m, uint32_t a) {
    uint32_t v;
    if (a & 3)
        Fault(m);
    memcpy(&v, MRef_Ptr(m, a), 4);
    return v;
}

static void Wr32(MRef *m, uint32_t a, uint32_t v) {
    if (a & 3)
        Fault(m);
    memcpy(MRef_Ptr(m, a), &v, 4);
}

uint32_t MRef_Call(MRef *m, uint32_t addr, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) {
    uint32_t next_pc = addr + 4;
    int ld_reg = 0, steps = 0;
    uint32_t ld_val = 0;
    jmp_buf jb;

    m->fault = 0;
    if (setjmp(jb)) {
        trap = NULL;
        return 0;
    }
    trap = &jb;

    m->r[4] = a0;
    m->r[5] = a1;
    m->r[6] = a2;
    m->r[7] = a3;
    m->r[29] = 0x801ffff0u;
    m->r[31] = RETURN_ADDR;
    m->pc = addr;

    while (m->pc != RETURN_ADDR) {
        uint32_t w = Rd32(m, m->pc);
        uint32_t op = w >> 26, rs = (w >> 21) & 31, rt = (w >> 16) & 31, rd = (w >> 11) & 31;
        uint32_t sh = (w >> 6) & 31, fn = w & 63;
        uint32_t s = m->r[rs], t = m->r[rt];
        uint32_t imm = (uint32_t)(int32_t)(int16_t)w, immu = w & 0xffff;
        uint32_t target = next_pc + (imm << 2);
        uint32_t pc_after = next_pc + 4;
        int wreg = 0, new_ld = 0; /* register this instruction writes; a new delayed load */
        uint32_t wval = 0, new_val = 0, ea = s + imm;
        /* a load issued by the previous instruction lands after this one has read its operands */
        int pend_reg = ld_reg;
        uint32_t pend_val = ld_val;

        if (++steps > MAX_STEPS)
            Die(m, "runaway call", w);
        ld_reg = 0;

        switch (op) {
        case 0x00:
            switch (fn) {
            case 0x00: wreg = rd; wval = t << sh; break;
            case 0x02: wreg = rd; wval = t >> sh; break;
            case 0x03: wreg = rd; wval = (uint32_t)((int32_t)t >> sh); break;
            case 0x04: wreg = rd; wval = t << (s & 31); break;
            case 0x06: wreg = rd; wval = t >> (s & 31); break;
            case 0x07: wreg = rd; wval = (uint32_t)((int32_t)t >> (s & 31)); break;
            case 0x08: pc_after = s; break;
            case 0x09: pc_after = s; wreg = rd; wval = next_pc + 4; break;
            case 0x10: wreg = rd; wval = m->hi; break;
            case 0x11: m->hi = s; break;
            case 0x12: wreg = rd; wval = m->lo; break;
            case 0x13: m->lo = s; break;
            case 0x18: {
                int64_t p = (int64_t)(int32_t)s * (int32_t)t;
                m->lo = (uint32_t)p;
                m->hi = (uint32_t)((uint64_t)p >> 32);
                break;
            }
            case 0x19: {
                uint64_t p = (uint64_t)s * t;
                m->lo = (uint32_t)p;
                m->hi = (uint32_t)(p >> 32);
                break;
            }
            case 0x1a:
                if (t == 0) {
                    m->lo = (int32_t)s >= 0 ? 0xffffffffu : 1u;
                    m->hi = s;
                } else if (s == 0x80000000u && t == 0xffffffffu) {
                    m->lo = s;
                    m->hi = 0;
                } else {
                    m->lo = (uint32_t)((int32_t)s / (int32_t)t);
                    m->hi = (uint32_t)((int32_t)s % (int32_t)t);
                }
                break;
            case 0x1b:
                m->lo = t ? s / t : 0xffffffffu;
                m->hi = t ? s % t : s;
                break;
            case 0x20:
                if ((int32_t)((s + t) ^ s) < 0 && (int32_t)((s + t) ^ t) < 0)
                    Fault(m);
                /* fall through */
            case 0x21: wreg = rd; wval = s + t; break;
            case 0x22:
                if ((int32_t)((s ^ t) & (s ^ (s - t))) < 0)
                    Fault(m);
                /* fall through */
            case 0x23: wreg = rd; wval = s - t; break;
            case 0x24: wreg = rd; wval = s & t; break;
            case 0x25: wreg = rd; wval = s | t; break;
            case 0x26: wreg = rd; wval = s ^ t; break;
            case 0x27: wreg = rd; wval = ~(s | t); break;
            case 0x2a: wreg = rd; wval = (int32_t)s < (int32_t)t; break;
            case 0x2b: wreg = rd; wval = s < t; break;
            default: Die(m, "unsupported SPECIAL instruction", w);
            }
            break;
        case 0x01: {
            int take = (rt & 1) ? (int32_t)s >= 0 : (int32_t)s < 0;
            if ((rt & 0x1e) == 0x10) {
                wreg = 31;
                wval = next_pc + 4;
            }
            if (take)
                pc_after = target;
            break;
        }
        case 0x02: pc_after = (next_pc & 0xf0000000u) | ((w & 0x3ffffff) << 2); break;
        case 0x03:
            pc_after = (next_pc & 0xf0000000u) | ((w & 0x3ffffff) << 2);
            wreg = 31;
            wval = next_pc + 4;
            break;
        case 0x04: if (s == t) pc_after = target; break;
        case 0x05: if (s != t) pc_after = target; break;
        case 0x06: if ((int32_t)s <= 0) pc_after = target; break;
        case 0x07: if ((int32_t)s > 0) pc_after = target; break;
        case 0x08:
            if ((int32_t)((s + imm) ^ s) < 0 && (int32_t)((s + imm) ^ imm) < 0)
                Fault(m);
            /* fall through */
        case 0x09: wreg = rt; wval = s + imm; break;
        case 0x0a: wreg = rt; wval = (int32_t)s < (int32_t)imm; break;
        case 0x0b: wreg = rt; wval = s < imm; break;
        case 0x0c: wreg = rt; wval = s & immu; break;
        case 0x0d: wreg = rt; wval = s | immu; break;
        case 0x0e: wreg = rt; wval = s ^ immu; break;
        case 0x0f: wreg = rt; wval = immu << 16; break;
        case 0x20: new_ld = rt; new_val = (uint32_t)(int32_t)*(int8_t *)MRef_Ptr(m, ea); break;
        case 0x24: new_ld = rt; new_val = *MRef_Ptr(m, ea); break;
        case 0x21:
        case 0x25: {
            uint16_t h;
            if (ea & 1)
                Fault(m);
            memcpy(&h, MRef_Ptr(m, ea), 2);
            new_ld = rt;
            new_val = op == 0x21 ? (uint32_t)(int32_t)(int16_t)h : h;
            break;
        }
        case 0x23: new_ld = rt; new_val = Rd32(m, ea); break;
        case 0x22:
        case 0x26: {
            /* lwl/lwr merge with the in-flight load of the same register, if any */
            uint32_t cur = (pend_reg == (int)rt) ? pend_val : t, word = Rd32(m, ea & ~3u);
            uint32_t k = ea & 3;
            if (op == 0x22)
                new_val = (cur & (0x00ffffffu >> (8 * k))) | (word << (8 * (3 - k)));
            else
                new_val = (cur & ~(0xffffffffu >> (8 * k))) | (word >> (8 * k));
            new_ld = rt;
            break;
        }
        case 0x28: *MRef_Ptr(m, ea) = (uint8_t)t; break;
        case 0x29: {
            uint16_t h = (uint16_t)t;
            if (ea & 1)
                Fault(m);
            memcpy(MRef_Ptr(m, ea), &h, 2);
            break;
        }
        case 0x2b: Wr32(m, ea, t); break;
        case 0x2a:
        case 0x2e: {
            uint32_t word = Rd32(m, ea & ~3u), k = ea & 3;
            if (op == 0x2a)
                word = (word & (0xffffff00u << (8 * k))) | (t >> (8 * (3 - k)));
            else
                word = (word & ~(0xffffffffu << (8 * k))) | (t << (8 * k));
            Wr32(m, ea & ~3u, word);
            break;
        }
        case 0x12:
            /* COP2: only moves to LZCS (data 30) and from LZCR (data 31). mfc2 lands an
             * instruction late, like a load. */
            if (rs == 0x04 && rd == 30 && (w & 0x7ff) == 0) {
                m->lzcs = t;
            } else if (rs == 0x00 && rd == 31 && (w & 0x7ff) == 0) {
                new_ld = rt;
                new_val = Lzcr(m->lzcs);
            } else {
                Die(m, "unsupported COP2 instruction", w);
            }
            break;
        default: Die(m, "unsupported instruction", w);
        }

        if (pend_reg && pend_reg != wreg)
            m->r[pend_reg] = pend_val;
        if (wreg)
            m->r[wreg] = wval;
        if (new_ld) {
            ld_reg = new_ld;
            ld_val = new_val;
        }
        m->r[0] = 0;
        m->pc = next_pc;
        next_pc = pc_after;
    }
    if (ld_reg)
        m->r[ld_reg] = ld_val;
    trap = NULL;
    return m->r[2];
}
