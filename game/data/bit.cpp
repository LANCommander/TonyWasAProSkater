/* Decompiled data: the retail executable's initialised globals that src/bit/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/m3d.h"

struct Texture;

/* THPS2 demo: char*[6] */
char *AnimNames[6] = {
	"SHADOW",
	"SMOKE",
	"orbit",
	"ribbon",
	"smtile",
	"Buttons",
};

/* THPS2 demo: SAnimFrame*[6] */
SAnimFrame *QuickAnimLookup[6];

/* SCamera[2] (THPS2 demo: SCamera[2]) */
int MikeCamera[82];

/* THPS2 demo: uchar* */
unsigned char *PolyBufferEnd;

/* THPS2 demo: Texture* */
Texture *pShadowTexture;

/* THPS2 demo: CSVector */
int SparkTrajectory[2];

/* CSVector (THPS2 demo: CSVector) */
int SparkTrajectoryCone[2];

/* THPS2 demo: uint */
unsigned int SparkSize[1] = {
	1,
};

/* THPS2 demo: null */
int SparkSemiTrans[1] = {
	1,
};

/* CVECTOR (THPS2 demo: CVECTOR) */
int SparkRGB[1] = {
	0x808080,
};

/* CVECTOR (THPS2 demo: CVECTOR) */
int SparkFadeRGB[1] = {
	0x8040404,
};

/* THPS2 demo: CChunkBit* */
void *ChunkBitList;

/* THPS2 demo: CGlow* */
void *GlowList;

/* THPS2 demo: CSpecialDisplay* */
void *SpecialDisplayList;

/* THPS2 demo: CGLine* */
void *GLineList;

/* StoredTPagePrim (0x800d1900, a DR_TPAGE) is Bit_Init's (src/bit.cpp): the port's primitives have a
   full-pointer tag, bigger than retail's 8 bytes (retail strings follow it) */

