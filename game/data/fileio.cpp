/* Decompiled data: the retail executable's initialised globals that src/fileio/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: char[22528] */
PSX_DATA(0x800ba09c) unsigned char Directory[16384];

/* THPS2 demo: int */
PSX_DATA(0x800d1b0c) int ReadSyncFailures[1];

/* THPS2 demo: char* */
PSX_DATA(0x800d1b18) char *ThePREFile;

/* THPS2 demo: int */
PSX_DATA(0x800d1b1c) int ThePREFileSize[1];

PSX_DATA(0x800d1b20) int DAT_800d1b20[2] = {
	0x1201e5, 0xa000a,
};

/* THPS2 demo: char* */
PSX_DATA(0x800d1b30) char *Opening;
