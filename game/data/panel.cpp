/* Decompiled data: the retail executable's initialised globals that src/panel/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

struct Sprite;

/* THPS2 demo: int */
PSX_DATA(0x800d1790) int GStartTime[1];

/* THPS2 demo: Sprite* */
PSX_DATA(0x800d184c) Sprite *pScoreSprite;

/* THPS2 demo: Sprite* */
PSX_DATA(0x800d1854) Sprite *pBarSprite;

/* THPS2 demo: int */
PSX_DATA(0x800d1e60) int TAPES_Pos[1] = {
	532,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1e64) int SKATE_Display[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1e68) int SKATE_Pos[1] = {
	532,
};

