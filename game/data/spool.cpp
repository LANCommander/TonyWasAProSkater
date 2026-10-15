/* Decompiled data: the retail executable's initialised globals that src/spool/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/spool.h"
#include "game/m3d.h"

struct AnimPacket;

/* SQueue[20] (THPS2 demo: SQueue[20]) */
SQueue SpoolQ[20];

/* THPS2 demo: Texture*[512] */
/* (with the space retail's tables after it shared) */
Texture *TextureChecksumHashTable[512];

/* THPS2 demo: int[2] */
int EnvRegions[2] = {
	-1, -1,
};

/* THPS2 demo: uint* */
unsigned int *pCurrentTexWibPacket;

/* THPS2 demo: uint*[2] */
uint *pTexWibPacket[2];

/* THPS2 demo: Item* */
Item *EnviroList;

/* THPS2 demo: int */
int SpoolPos[1];

/* THPS2 demo: int */
int FreePos[1];

/* THPS2 demo: AnimPacket* */
AnimPacket *AnimPackets;

int DAT_800d1f08[1];
