/* Support code for C translated from MIPS by tools/mips2c.py. */
#ifndef PC_RECOMP_H
#define PC_RECOMP_H

#include <stdint.h>
#include "pc_gte.h"
#include "pc_runtime.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/* The shared MIPS register file: r[0..31], then HI and LO. */
typedef struct {
    u32 r[32];
    u32 hi, lo;
} PcRegs;
extern PcRegs pc_r;

typedef struct {
    u32 addr;
    void (*body)(void);
} PcRecompBody;

/* PS1 memory: main RAM and the scratchpad are mapped at their own addresses. */
#define MEM32(a) (*(volatile u32 *)(uintptr_t)(u32)(a))
#define MEM16(a) (*(volatile u16 *)(uintptr_t)(u32)(a))
#define MEM16S(a) (*(volatile s16 *)(uintptr_t)(u32)(a))
#define MEM8(a) (*(volatile u8 *)(uintptr_t)(u32)(a))
#define MEM8S(a) (*(volatile s8 *)(uintptr_t)(u32)(a))

#define REGS_DECL                                                                                  \
    u32 at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6,     \
        s7, t8, t9, k0, k1, gp, sp, fp, ra, hi, lo;
#define REGS_IN()                                                                                  \
    do {                                                                                           \
        at = pc_r.r[1]; v0 = pc_r.r[2]; v1 = pc_r.r[3]; a0 = pc_r.r[4]; a1 = pc_r.r[5];            \
        a2 = pc_r.r[6]; a3 = pc_r.r[7]; t0 = pc_r.r[8]; t1 = pc_r.r[9]; t2 = pc_r.r[10];           \
        t3 = pc_r.r[11]; t4 = pc_r.r[12]; t5 = pc_r.r[13]; t6 = pc_r.r[14]; t7 = pc_r.r[15];       \
        s0 = pc_r.r[16]; s1 = pc_r.r[17]; s2 = pc_r.r[18]; s3 = pc_r.r[19]; s4 = pc_r.r[20];       \
        s5 = pc_r.r[21]; s6 = pc_r.r[22]; s7 = pc_r.r[23]; t8 = pc_r.r[24]; t9 = pc_r.r[25];       \
        k0 = pc_r.r[26]; k1 = pc_r.r[27]; gp = pc_r.r[28]; sp = pc_r.r[29]; fp = pc_r.r[30];       \
        ra = pc_r.r[31]; hi = pc_r.hi; lo = pc_r.lo;                                               \
    } while (0)
#define REGS_OUT()                                                                                 \
    do {                                                                                           \
        pc_r.r[1] = at; pc_r.r[2] = v0; pc_r.r[3] = v1; pc_r.r[4] = a0; pc_r.r[5] = a1;            \
        pc_r.r[6] = a2; pc_r.r[7] = a3; pc_r.r[8] = t0; pc_r.r[9] = t1; pc_r.r[10] = t2;           \
        pc_r.r[11] = t3; pc_r.r[12] = t4; pc_r.r[13] = t5; pc_r.r[14] = t6; pc_r.r[15] = t7;       \
        pc_r.r[16] = s0; pc_r.r[17] = s1; pc_r.r[18] = s2; pc_r.r[19] = s3; pc_r.r[20] = s4;       \
        pc_r.r[21] = s5; pc_r.r[22] = s6; pc_r.r[23] = s7; pc_r.r[24] = t8; pc_r.r[25] = t9;       \
        pc_r.r[26] = k0; pc_r.r[27] = k1; pc_r.r[28] = gp; pc_r.r[29] = sp; pc_r.r[30] = fp;       \
        pc_r.r[31] = ra; pc_r.hi = hi; pc_r.lo = lo;                                               \
    } while (0)

static inline void rc_div(u32 *lo, u32 *hi, u32 a, u32 b) {
    s32 n = (s32)a, d = (s32)b;
    if (d == 0) {
        *lo = n >= 0 ? 0xffffffffu : 1u;
        *hi = (u32)n;
    } else if (n == INT32_MIN && d == -1) {
        *lo = (u32)INT32_MIN;
        *hi = 0;
    } else {
        *lo = (u32)(n / d);
        *hi = (u32)(n % d);
    }
}

static inline void rc_divu(u32 *lo, u32 *hi, u32 a, u32 b) {
    if (b == 0) {
        *lo = 0xffffffffu;
        *hi = a;
    } else {
        *lo = a / b;
        *hi = a % b;
    }
}

static inline u32 rc_lwl(u32 rt, u32 addr) {
    u32 sh = (addr & 3) * 8, w = MEM32(addr & ~3u);
    return (rt & (0x00ffffffu >> sh)) | (w << (24 - sh));
}
static inline u32 rc_lwr(u32 rt, u32 addr) {
    u32 sh = (addr & 3) * 8, w = MEM32(addr & ~3u);
    return (rt & ~(0xffffffffu >> sh)) | (w >> sh);
}
static inline void rc_swl(u32 rt, u32 addr) {
    u32 sh = (addr & 3) * 8, a = addr & ~3u;
    MEM32(a) = (MEM32(a) & ~(0xffffffffu >> (24 - sh))) | (rt >> (24 - sh));
}
static inline void rc_swr(u32 rt, u32 addr) {
    u32 sh = (addr & 3) * 8, a = addr & ~3u;
    MEM32(a) = (MEM32(a) & ~(0xffffffffu << sh)) | (rt << sh);
}

u32 rc_enter(void (*body)(void), u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7);
void rc_call_native(u32 target);
void rc_call_indirect(u32 target);
u32 rc_cop0_read(int reg);
void rc_break(u32 pc);
void rc_bad_jump(u32 pc, u32 target);
void rc_untranslatable(u32 entry);
u32 PC_BiosCall(u32 vector, u32 fn);
void PC_Syscall(void);

#endif
