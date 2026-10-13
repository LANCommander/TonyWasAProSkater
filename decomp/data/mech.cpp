/* Decompiled data: the retail executable's initialised globals that src/mech/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

PSX_DATA(0x800a8118) int DAT_800a8118[13] = {
	0x10000000, 0, 0xf000, 0, 4096, 0xb800280, 0x2800140, 0x1400b80, 0xb800280, 320, 2176, 2176,
	2176,
};

/* THPS2 demo: short[14][10] */
PSX_DATA(0x800a814c) short SkaterHooks[110] = {
	0, 1, 0, 0, 0, 18, 0, 0, 0, 3, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 12, 0, 0, 0, 0, 0, 1, 0, 0, 0, 12,
};

/* THPS2 demo: int */
PSX_DATA(0x800d16f4) int CurrentWaterLevel2[1] = {
	0x7fffffff,
};

/* THPS2 demo: int */
PSX_DATA(0x800d16f8) int CurrentDualShockLevel[1] = {
	2,
};

/* THPS2 demo: int */
PSX_DATA(0x800d171c) int Cheat_Blood[1] = {
	1,
};

/* THPS2 demo: int */
PSX_DATA(0x800d19a0) int NumMechs[1];
