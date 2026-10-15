/* Decompiled data: the retail executable's initialised globals that src/m3dinit/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: int */
PSX_DATA(0x800d2124) int GBoardPart[1] = {
	-1,
};

/* THPS2 demo: int */
PSX_DATA(0x800d2128) int GWheel1Part[1] = {
	-1,
};

/* THPS2 demo: int */
PSX_DATA(0x800d212c) int GWheel2Part[1] = {
	-1,
};

/* THPS2 demo: int */
PSX_DATA(0x800d213c) int LastDpqShift[1];

/* THPS2 demo: short */
PSX_DATA(0x800d2158) short M3d_DefaultLODDistance[2] = {
	0x1770,
};
