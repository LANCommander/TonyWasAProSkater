/* Decompiled data that src/medals/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char DeleteStuff__6CMedal[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_80073da8[];
extern char dtor_6CMedal[] PSX_NAME("_._6CMedal");

PSX_FIXED(0x800a314c) int __vt_6CMedal[16] = {
	0, 0, 0, (int)dtor_6CMedal, 0, (int)Die__5CBody, 0, (int)FUN_80073da8, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)DeleteStuff__6CMedal,
};
