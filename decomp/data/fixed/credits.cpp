/* Decompiled data that src/credits/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char dtor_5CItem_8005bb3c[] PSX_NAME("_._5CItem_8005bb3c");

PSX_FIXED(0x800a18e4) int __vt_5CItem_800a18e4[4] = {
	0, 0, 0, (int)dtor_5CItem_8005bb3c,
};
