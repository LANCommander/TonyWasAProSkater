/* Decompiled data that src/backgrnd/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char AI__11CBackground[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char dtor_11CBackground[] PSX_NAME("_._11CBackground");

PSX_FIXED(0x800a3494) int __vt_11CBackground[16] = {
	0, 0, 0, (int)dtor_11CBackground, 0, (int)Die__5CBody, 0, (int)AI__11CBackground, 0,
	(int)FUN_800447e4, 0, (int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4,
};
