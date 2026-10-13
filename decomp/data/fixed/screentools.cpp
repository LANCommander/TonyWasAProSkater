/* Decompiled data that src/screentools/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char Display__14CGouraudRibbon[];
extern char FUN_8002ee64[];
extern char dtor_14CGouraudRibbon[] PSX_NAME("_._14CGouraudRibbon");

PSX_FIXED(0x8009f708) int D_8009f708[8] = {
	0, 0, 0, (int)dtor_14CGouraudRibbon, 0, (int)FUN_8002ee64, 0, (int)Display__14CGouraudRibbon,
};
