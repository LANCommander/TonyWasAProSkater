/* Decompiled data that src/mech/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char Die__6CBruce[];
extern char FUN_80042380[];
extern char FUN_800425b8[];
extern char FUN_800427bc[];
extern char FUN_800427c4[];
extern char FUN_80042d04[];
extern char FUN_800447e4[];

PSX_FIXED(0x800a0534) int DAT_800a0534[16] = {
	0, 0, 0, (int)FUN_80042380, 0, (int)Die__6CBruce, 0, (int)FUN_80042d04, 0, (int)FUN_800447e4, 0,
	(int)FUN_800425b8, 0, (int)FUN_800427c4, 0, (int)FUN_800427bc,
};
