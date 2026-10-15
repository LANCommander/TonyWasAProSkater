/* Decompiled data: the retail executable's initialised globals that src/video/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/video.h"

/* the two players' replays: GVInfo1 and GVInfo2 (THPS2 demo: SVideoInfo each); the code picks
   player 2's as GVInfo1[1] */
SVideoInfo GVInfo1[2];

/* THPS2 demo: int[7] */
int PosBits[7] = {
	3, 4, 5, 7, 8, 16, 20,
};

/* THPS2 demo: int[7] */
int EulerBits[7] = {
	2, 4, 5, 6, 7, 9, 12,
};

/* THPS2 demo: int[7] */
int FlipAngleBits[7] = {
	0, 6, 7, 8, 12, -1, -1,
};

/* THPS2 demo: int[7] */
int PrefixCode[7] = {
	0, 1, 3, 7, 15, 31, 63,
};

/* THPS2 demo: int[7] */
int PrefixBits[7] = {
	1, 2, 3, 4, 5, 6, 6,
};

/* THPS2 demo: SSoundCmd[30] */
int SoundCmd[60];

unsigned char DAT_800d000b[1];

/* THPS2 demo: int */
int ReplaySoundLockout[1];

/* THPS2 demo: int */
int GVideoReplay[1];

/* THPS2 demo: int */
int HLStartCalled[1];

int DAT_800d1df0[1];

/* THPS2 demo: int */
int Splat1[1];

/* THPS2 demo: int */
int Splat2[1];

/* THPS2 demo: int */
int HighlightStartFrame[1];

/* THPS2 demo: int */
int HighlightEndFrame[1];

/* THPS2 demo: int */
int HighlightStartY[1];

/* THPS2 demo: int */
int HighlightEndY[2];
