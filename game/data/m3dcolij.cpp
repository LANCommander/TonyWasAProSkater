/* Decompiled data: the retail executable's initialised globals that src/m3dcolij/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/colij.h"

struct CBody;

/* SHit (THPS2 demo: SHit) */
PSX_DATA(0x800c1304) SHit NearestHit;

/* THPS2 demo: SLineInfo */
PSX_DATA(0x800c1328) int GColInfo[41];

/* MATRIX (THPS2 demo: MATRIX) */
PSX_DATA(0x800c13cc) int Tfm[8];

/* SColCache[10] (THPS2 demo: SColCache[20]) */
PSX_DATA(0x800c13ec) SColCache ItemCache[10];

/* SFaceCache[500] (THPS2 demo: SFaceCache[500]) */
PSX_DATA(0x800c148c) int FaceCache[3500];

/* THPS2 demo: int */
PSX_DATA(0x800d1ef0) int OtherEnvIndex[1];

/* THPS2 demo: uint*[2] */
PSX_DATA(0x800d1f0c) int *EnvModelChecksums[2];

/* THPS2 demo: ushort */
PSX_DATA(0x800d20c8) unsigned short Inquiry[2] = {
	0xffff,
};

/* THPS2 demo: int */
PSX_DATA(0x800d20d8) int TriggerCollisionCheck[1];

/* THPS2 demo: int */
PSX_DATA(0x800d20e0) int ItemCacheIndex[1];

PSX_DATA(0x800d20e4) int DAT_800d20e4[1];

PSX_DATA(0x800d20e8) int DAT_800d20e8[5] = {
	-1,
};

/* THPS2 demo: CBody* */
PSX_DATA(0x800d2108) CBody *pBodyHit;

PSX_DATA(0x800d2110) int DAT_800d2110[1];
PSX_DATA(0x800d2114) int DAT_800d2114[1];
PSX_DATA(0x800d2118) int DAT_800d2118[1];
PSX_DATA(0x800d211c) int DAT_800d211c[1];
