/* Decompiled data: the retail executable's initialised globals that src/spool/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* SQueue[20] (THPS2 demo: SQueue[20]) */
PSX_DATA(0x800bf654) int SpoolQ[85];

/* THPS2 demo: Texture*[512] */
PSX_DATA(0x800c01a8) unsigned char TextureChecksumHashTable[165];

/* THPS2 demo: int[2] */
PSX_DATA(0x800d1ed0) int EnvRegions[2] = {
	-1, -1,
};

/* THPS2 demo: uint* */
PSX_DATA(0x800d1ee0) int pCurrentTexWibPacket[1];

/* THPS2 demo: uint*[2] */
PSX_DATA(0x800d1ee4) int pTexWibPacket[2];

/* THPS2 demo: Item* */
PSX_DATA(0x800d1eec) int EnviroList[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1ef4) int SpoolPos[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1ef8) int FreePos[1];

/* THPS2 demo: AnimPacket* */
PSX_DATA(0x800d1efc) int AnimPackets[1];

PSX_DATA(0x800d1f08) int DAT_800d1f08[1];
