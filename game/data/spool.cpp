/* Decompiled data: the retail executable's initialised globals that src/spool/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/spool.h"
#include "game/m3d.h"

struct AnimPacket;

/* SQueue[20] (THPS2 demo: SQueue[20]) */
PSX_DATA(0x800bf654) SQueue SpoolQ[20];

/* THPS2 demo: Texture*[512] */
/* (with the space retail's tables after it shared) */
PSX_DATA(0x800c01a8) Texture *TextureChecksumHashTable[512];

/* THPS2 demo: int[2] */
PSX_DATA(0x800d1ed0) int EnvRegions[2] = {
	-1, -1,
};

/* THPS2 demo: uint* */
PSX_DATA(0x800d1ee0) unsigned int *pCurrentTexWibPacket;

/* THPS2 demo: uint*[2] */
PSX_DATA(0x800d1ee4) uint *pTexWibPacket[2];

/* THPS2 demo: Item* */
PSX_DATA(0x800d1eec) Item *EnviroList;

/* THPS2 demo: int */
PSX_DATA(0x800d1ef4) int SpoolPos[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1ef8) int FreePos[1];

/* THPS2 demo: AnimPacket* */
PSX_DATA(0x800d1efc) AnimPacket *AnimPackets;

PSX_DATA(0x800d1f08) int DAT_800d1f08[1];
