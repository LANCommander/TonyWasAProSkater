/* Decompiled data: the retail executable's initialised globals that src/init/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: ushort* */
PSX_DATA(0x800d114c) void *pCompressedBMR;

/* THPS2 demo: int */
PSX_DATA(0x800d13d4) int TitleTimeout[1] = {
	20,
};
