/* Decompiled data that src/shatter/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char FUN_80020c90[];
extern char Move__11CShatterBit[];

PSX_FIXED(0x8009f280) int __vt_11CShatterBit[6] = {
	0, 0, 0, (int)FUN_80020c90, 0, (int)Move__11CShatterBit,
};
