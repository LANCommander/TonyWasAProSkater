/* Decompiled data that src/front/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char FUN_80068aac[];

PSX_FIXED(0x800a2310) int DAT_800a2310[4] = {
	0, 0, 0, (int)FUN_80068aac,
};
