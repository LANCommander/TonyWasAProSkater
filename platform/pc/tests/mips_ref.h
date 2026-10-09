/* A small, independent R3000 interpreter for differential tests: it runs functions straight from
 * the retail executable's bytes, so C reimplementations of hand-written assembly can be compared
 * with the original code. Integer ISA with branch and load delay slots, the scratchpad, and of the
 * GTE only LZCS/LZCR (the leading-bit counter hand-written code uses); no COP0.
 * Deliberately shares nothing with tools/mips2c.py, so it checks the translator too. */
#ifndef MIPS_REF_H
#define MIPS_REF_H

#include <stdint.h>

#define MREF_RAM_SIZE 0x200000u
#define MREF_SCRATCH_SIZE 0x400u

typedef struct {
    uint32_t r[32], hi, lo, pc;
    uint32_t lzcs;  /* GTE data register 30 */
    uint8_t *ram;   /* 2 MB at 0x80000000 (KUSEG 0x00000000 and KSEG1 0xa0000000 mirror it) */
    uint8_t scratch[MREF_SCRATCH_SIZE]; /* 1 KB at 0x1f800000 */
    int fault;      /* set by MRef_Call when the code did something the PS1 would bus-error on */
} MRef;

/* Load a PS-X EXE into a fresh machine. Returns 0 on failure (missing file, bad header). */
int MRef_Load(MRef *m, const char *exe_path);
void MRef_Free(MRef *m);

/* RAM access by PS1 address, for setting up arguments and reading results. */
uint8_t *MRef_Ptr(MRef *m, uint32_t addr);

/* Call the function at `addr` with up to four arguments; returns v0. The stack starts below the
 * top of RAM. An access outside RAM and the scratchpad, an unaligned access or an overflow trap
 * ends the call with m->fault set; an unsupported instruction or a runaway call aborts the test
 * program. */
uint32_t MRef_Call(MRef *m, uint32_t addr, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);

#endif
