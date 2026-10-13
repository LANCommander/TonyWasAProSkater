/* Decompiled data: the retail executable's initialised globals that src/bit/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: char*[6] */
PSX_DATA(0x800a7220) char *AnimNames[6] = {
	"SHADOW",
	"SMOKE",
	"orbit",
	"ribbon",
	"smtile",
	"Buttons",
};

/* THPS2 demo: SAnimFrame*[6] */
PSX_DATA(0x800a7238) int QuickAnimLookup[6];

/* SCamera[2] (THPS2 demo: SCamera[2]) */
PSX_DATA(0x800c119c) int MikeCamera[82];

/* THPS2 demo: uchar* */
PSX_DATA(0x800d177c) int PolyBufferEnd[1];

/* THPS2 demo: Texture* */
PSX_DATA(0x800d18a0) int pShadowTexture[1];

/* THPS2 demo: CSVector */
PSX_DATA(0x800d18a4) int SparkTrajectory[2];

/* CSVector (THPS2 demo: CSVector) */
PSX_DATA(0x800d18ac) int SparkTrajectoryCone[2];

/* THPS2 demo: uint */
PSX_DATA(0x800d18b4) unsigned int SparkSize[1] = {
	1,
};

/* THPS2 demo: null */
PSX_DATA(0x800d18b8) int SparkSemiTrans[1] = {
	1,
};

/* CVECTOR (THPS2 demo: CVECTOR) */
PSX_DATA(0x800d18bc) int SparkRGB[1] = {
	0x808080,
};

/* CVECTOR (THPS2 demo: CVECTOR) */
PSX_DATA(0x800d18c0) int SparkFadeRGB[1] = {
	0x8040404,
};

/* THPS2 demo: CChunkBit* */
PSX_DATA(0x800d18e4) int ChunkBitList[1];

/* THPS2 demo: CGlow* */
PSX_DATA(0x800d18e8) int GlowList[1];

/* THPS2 demo: CSpecialDisplay* */
PSX_DATA(0x800d18f0) int SpecialDisplayList[1];

/* THPS2 demo: CGLine* */
PSX_DATA(0x800d18fc) int GLineList[1];

/* DR_TPAGE (THPS2 demo: DR_TPAGE) */
PSX_DATA(0x800d1900) int StoredTPagePrim[2];

