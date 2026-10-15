/* Decompiled data: the retail executable's initialised globals that src/m3dcolij/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/colij.h"

struct CBody;

/* SHit (THPS2 demo: SHit) */
SHit NearestHit;

/* THPS2 demo: SLineInfo */
int GColInfo[41];

/* MATRIX (THPS2 demo: MATRIX) */
int Tfm[8];

/* SColCache[10] (THPS2 demo: SColCache[20]) */
SColCache ItemCache[10];

/* SFaceCache[500] (THPS2 demo: SFaceCache[500]) */
int FaceCache[3500];

/* THPS2 demo: int */
int OtherEnvIndex[1];

/* THPS2 demo: uint*[2] */
int *EnvModelChecksums[2];

/* THPS2 demo: ushort */
unsigned short Inquiry[2] = {
	0xffff,
};

/* THPS2 demo: int */
int TriggerCollisionCheck[1];

/* THPS2 demo: int */
int ItemCacheIndex[1];

int DAT_800d20e4[1];

int DAT_800d20e8[5] = {
	-1,
};

/* THPS2 demo: CBody* */
CBody *pBodyHit;

int DAT_800d2110[1];
int DAT_800d2114[1];
int DAT_800d2118[1];
int DAT_800d211c[1];
