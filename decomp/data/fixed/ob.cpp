/* Decompiled data that src/ob/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_800447ec[];
extern char dtor_5CBody[] PSX_NAME("_._5CBody");
extern char dtor_6CSuper[] PSX_NAME("_._6CSuper");

PSX_FIXED(0x800a0574) int __vt_6CSuper[16] = {
	0, 0, 0, (int)dtor_6CSuper, 0, (int)Die__5CBody, 0, (int)FUN_800447ec, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4,
};

PSX_FIXED(0x800a05b4) int __vt_5CBody[16] = {
	0, 0, 0, (int)dtor_5CBody, 0, (int)Die__5CBody, 0, (int)FUN_800447ec, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4,
};
