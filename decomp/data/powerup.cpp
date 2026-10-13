/* Decompiled data: the retail executable's initialised globals that src/powerup/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: SMessageProg[5] */
PSX_DATA(0x800a5fd0) int Messprog_Zoom[40] = {
	0x10005, 0x1800000, -1, 0xffff, 0x1000000, 0x100005a, 90, 0x10000, 0x10001, 0x1800180, -1,
	0xffff, 0x1000000, 0x100005a, 90, 0x10000, 0x10004, 0x1000180, -1, 0xffff, 0x1000000, 0x100005a,
	90, 0x10000, 0x1000a, 0x1000100, -1, 0xffff, 0x1000000, 0x100005a, 90, 0x10000, 0x1000f, 256,
	-1, 0xffff, 0x1000000, 0x100005a, 90,
};

/* THPS2 demo: uchar */
PSX_DATA(0x800d1a15) unsigned char CurrentItemsFileRegion[1];

/* THPS2 demo: uchar */
PSX_DATA(0x800d1a16) unsigned char CurrentMedalsFileRegion[2];
