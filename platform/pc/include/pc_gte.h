/* Software GTE (PS1 COP2), register level (src/pc_gte.c): the reference the port's geometry maths
 * (pc_geom.h) is tested against, and the GTE of code translated from MIPS by mips2c
 * (mtc2/mfc2/ctc2/cfc2/lwc2/swc2/cop2) in build.py --mips builds (PC_MIPS). The port itself has no
 * GTE. Semantics follow psx-spx "Geometry Transformation Engine". */
#ifndef PC_GTE_H
#define PC_GTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Data registers 0-31 (mfc2/mtc2, lwc2/swc2): read/write side effects included (SXYP pushes the
 * FIFO, IRGB/ORGB convert, LZCS sets LZCR, 16-bit registers sign/zero extend on read). */
uint32_t PC_GteGetData(int reg);
void PC_GteSetData(int reg, uint32_t value);

/* Control registers 0-31 (cfc2/ctc2), with the hardware's sign-extension of 16-bit fields. */
uint32_t PC_GteGetCtrl(int reg);
void PC_GteSetCtrl(int reg, uint32_t value);

/* Execute a GTE command: the low 25 bits of the cop2 instruction (sf, mx, v, cv, lm, opcode). */
void PC_GteCommand(uint32_t op);

/* Reset all registers to zero (power-on state); the control registers are pc_geom's. */
void PC_GteReset(void);

/* The whole register state as an opaque blob (differential tests, snapshots). */
int PC_GteStateSize(void);
void PC_GteSaveState(void *buf);
void PC_GteLoadState(const void *buf);

#ifdef __cplusplus
}
#endif

#endif
