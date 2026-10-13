/* Decompiled data that src/camera/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char AI__7CCamera[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_8007d944[];
extern char FUN_8007d978[];
extern char dtor_7CCamera[] PSX_NAME("_._7CCamera");

PSX_FIXED(0x800a3554) int D_800a3554[16] = {
	0, 0, 0, (int)dtor_7CCamera, 0, (int)Die__5CBody, 0, (int)AI__7CCamera, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)FUN_800447d4,
};

PSX_FIXED(0x800a35b4) int D_800a35b4[4] = {
	0, 0, 0, (int)FUN_8007d944,
};

PSX_FIXED(0x800a35c4) int D_800a35c4[4] = {
	0, 0, 0, (int)FUN_8007d978,
};
