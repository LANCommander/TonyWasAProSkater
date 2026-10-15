/* Decompiled data: the retail executable's initialised globals that src/video/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/video.h"

/* the two players' replays: GVInfo1 and GVInfo2 (THPS2 demo: SVideoInfo each); the code picks
   player 2's as GVInfo1[1] */
PSX_DATA(0x800bf318) SVideoInfo GVInfo1[2];

/* THPS2 demo: int[7] */
PSX_DATA(0x800bf498) int PosBits[7] = {
	3, 4, 5, 7, 8, 16, 20,
};

/* THPS2 demo: int[7] */
PSX_DATA(0x800bf4b4) int EulerBits[7] = {
	2, 4, 5, 6, 7, 9, 12,
};

/* THPS2 demo: int[7] */
PSX_DATA(0x800bf4d0) int FlipAngleBits[7] = {
	0, 6, 7, 8, 12, -1, -1,
};

/* THPS2 demo: int[7] */
PSX_DATA(0x800bf4ec) int PrefixCode[7] = {
	0, 1, 3, 7, 15, 31, 63,
};

/* THPS2 demo: int[7] */
PSX_DATA(0x800bf508) int PrefixBits[7] = {
	1, 2, 3, 4, 5, 6, 6,
};

/* THPS2 demo: SSoundCmd[30] */
PSX_DATA(0x800bf524) int SoundCmd[60];

PSX_DATA(0x800d000b) unsigned char DAT_800d000b[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1de4) int ReplaySoundLockout[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1de8) int GVideoReplay[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1dec) int HLStartCalled[1];

PSX_DATA(0x800d1df0) int DAT_800d1df0[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1dfc) int Splat1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e00) int Splat2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e0c) int HighlightStartFrame[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e10) int HighlightEndFrame[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e14) int HighlightStartY[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e18) int HighlightEndY[2];
