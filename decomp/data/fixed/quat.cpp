/* Decompiled data that src/quat/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char FUN_80078088[];
extern char FUN_800780bc[];

PSX_FIXED(0x800a3474) int D_800a3474[4] = {
	0, 0, 0, (int)FUN_80078088,
};

PSX_FIXED(0x800a3484) int D_800a3484[4] = {
	0, 0, 0, (int)FUN_800780bc,
};
