/* Decompiled data: the retail executable's initialised globals that src/mem/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

struct SBlockHeader;

PSX_DATA(0x800d1b38) int DAT_800d1b38[2] = {
	100, 0x1000032,
};

PSX_DATA(0x800d1b40) char *DAT_800d1b40;
PSX_DATA(0x800d1b44) char *DAT_800d1b44;

/* THPS2 demo: int */
PSX_DATA(0x800d1b50) int LowMemory;

/* THPS2 demo: uint */
PSX_DATA(0x800d1b58) unsigned int UniqueIndentifier;

/* THPS2 demo: SBlockHeader*[2] */
PSX_DATA(0x800d1b64) SBlockHeader *FirstFreeBlock[2];
