/* Decompiled data that src/shellutils/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char dtor_12VerticalMenu[] PSX_NAME("_._12VerticalMenu");
extern char draw__12VerticalMenu[];

PSX_FIXED(0x800a2158) int D_800a2158[6] = {
	0, 0, 0, (int)dtor_12VerticalMenu, 0, (int)draw__12VerticalMenu,
};
