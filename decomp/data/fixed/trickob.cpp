/* Decompiled data that src/trickob/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char FUN_8004ef80[];

PSX_FIXED(0x800a1320) int __vt_8CTrickOb[4] = {
	0, 0, 0, (int)FUN_8004ef80,
};
