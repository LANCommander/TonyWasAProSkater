/* Decompiled data: the retail executable's initialised globals that src/physics/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/panel.h"

PSX_DATA(0x800a7250) STrick DAT_800a7250 = {
	0, 60, 0x10000, "VARIAL", 0, 9, 9, 800,
};

PSX_DATA(0x800a727c) STrick DAT_800a727c = {
	0, 61, 0x10000, "FINGER FLIP", 26, 8, 8, 500,
};

PSX_DATA(0x800a72a8) STrick DAT_800a72a8 = {
	0, 59, 0x10000, "FRONT FOOT IMPOSSIBLE", 0, 8, 8, 600,
};

PSX_DATA(0x800a72d4) STrick DAT_800a72d4 = {
	0x1000000, 61, 0x10000, "SEX CHANGE", 0, 6, 8, 500,
};

PSX_DATA(0x800a7300) STrick DAT_800a7300 = {
	0, 60, 0x10000, "HARDFLIP", 0, 4, 8, 300,
};

PSX_DATA(0x800a732c) STrick DAT_800a732c = {
	0, 59, 0x10000, "360 FLIP", 0, 3, 8, 300,
};

/* THPS2 demo: SGroup[4] */
PSX_DATA(0x800a7358) STrick Group[6] = {
	{0, 78, -1},
	{0, 53, 0x10000, "360 SHOVE IT", 0, 7, 10, 250},
	{0, 22, 0x10000, "IMPOSSIBLE", 0, 3, 6, 250},
	{0, 78, -1},
	{0, 20, 0x10000, "KICKFLIP", 0, 6, 10, 100},
	{0, 28, 0x10000, "KICKFLIP TO INDY", 28, 8, 0, 500},
};

PSX_DATA(0x800a7460) STrick DAT_800a7460[3] = {
	{0, 78, -1},
	{0, 78, -1},
	{0, 21, 0x10000, "HEELFLIP", 0, 6, 10, 100},
};

PSX_DATA(0x800a74e4) int DAT_800a74e4[11] = {
	0, 78, -1,
};

PSX_DATA(0x800a7510) STrick DAT_800a7510[53] = {
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 31, 0x10000, "JAPAN AIR", 0, 4, 0, 350},
	{0, 32, 0x10000, "TAILGRAB", 0, 5, 0, 300},
	{0, 78, -1},
	{0, 18, 0x18000, "METHOD", 0, 5, 0, 300},
	{0, 30, 0x10000, "MADONNA", 0, 4, 0, 500},
	{0, 29, 0x10000, "STALEFISH", 0, 4, 0, 300},
	{0, 78, -1},
	{0, 19, 0x18000, "INDY NOSEBONE", 0, 6, 0, 300},
	{0, 24, 0x10000, "ROCKET AIR", 0, 4, 0, 350},
	{0, 23, 0x10000, "BENIHANA", 14, 4, 0, 400},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0, 78, -1},
	{0xf00821, 65, 0x10000, "{THE 900}", 0, 5, 0, 8000},
	{0xf00881, 62, 0x10000, "{KICKFLIP MCTWIST}", 0, 5, 0, 4000},
	{0x442, 63, 0x10000, "{540 BOARD VARIAL}", 0, 5, 0, 2000, 0, 128},
	{0x282, 64, 0x10000, "{360 FLIP TO MUTE}", 0, 5, 0, 1500, 0, 128},
	{0xf03881, 62, 0x10000, "{KICKFLIP MCTWIST}", 0, 5, 0, 4000, 0, 100},
	{0x803481, 63, 0x10000, "{FINGERFLIP AIRWALK}", 0, 10, 0, 2000, 0, 128},
	{0xf03212, 64, 0x10000, "{VARIAL HEELFLIP JUDO}", 0, 5, 0, 2500},
	{0x4101121, 62, 0x10000, "{BACKFLIP}", 0, 0, 0, 4000, 1, 120},
	{0x1883, 64, 0x10000, "{ONE FOOTED SMITH}", 0, 5, 5, 750},
	{0x2001413, 63, 0x18000, "{BURNTWIST}", 0, 10, 10, 5000, 0, 128},
	{0xf07881, 62, 0x10000, "{KICKFLIP MCTWIST}", 0, 5, 0, 4000, 0, 100},
	{0x7481, 63, 0x10000, "{CHRIST AIR}", 18, 0, 0, 2000},
	{0x7122, 64, 0x10000, "{FRONT-BACK KICKFLIP}", 34, 5, 0, 1500, 0, 128},
	{0x4104211, 62, 0x10000, "{FRONTFLIP}", 0, 0, 0, 4000, 1, -120},
	{0x4823, 63, 0x10000, "{ONE FOOT 5-0 THUMPIN}", 0, 10, 10, 750},
	{0x4882, 64, 0x10000, "{360 SHOVE IT REWIND}", 0, 10, 10, 1500},
	{0x4102121, 62, 0x10000, "{BACKFLIP}", 0, 0, 0, 4000, 1, 120},
	{0x2822, 63, 0x10000, "{DOUBLE HARDFLIP}", 0, 10, 10, 1500},
	{0x2483, 64, 0x10000, "{DARKSLIDE}", 25, 10, 10, 750, 0, 128},
	{0x4105211, 62, 0x10000, "{FRONTFLIP}", 0, 0, 0, 4000, 1, -120},
	{0x5123, 63, 0x10000, "{CASPER SLIDE}", 16, 8, 10, 750},
	{0x5482, 64, 0x10000, "{KICKFLIP UNDERFLIP}", 0, 10, 10, 1500, 0, 128},
	{0x4106121, 62, 0x10000, "{BACKFLIP}", 0, 0, 0, 4000, 1, 120},
	{0x6223, 63, 0x10000, "{HEELFLIP TO BLUNTSLIDE}", 23, 10, 10, 750},
	{0x6442, 64, 0x10000, "{TRIPLE KICKFLIP}", 0, 10, 10, 1500, 0, 128},
	{0x4108211, 62, 0x10000, "{FRONTFLIP}", 0, 0, 0, 4000, 1, -120},
	{0x8113, 63, 0x10000, "{ONE FOOTED NOSEGRIND}", 0, 10, 10, 750},
	{0x8422, 64, 0x10000, "{540 FLIP}", 0, 10, 10, 1500, 0, 128},
	{0x4109121, 62, 0x10000, "{BACKFLIP}", 0, 0, 0, 4000, 1, 120},
	{0x9443, 63, 0x10000, "{PRIMO GRIND}", 20, 10, 10, 750},
	{0x809421, 64, 0x10000, "{JUDO MADONNA}", 0, 5, 0, 1500, 0, 128},
};

PSX_DATA(0x800a7edc) int DAT_800a7edc[5] = {
	0xfa00c8, 0x640064, 0x7d007d, 0x960064, 0xfa0096,
};

PSX_DATA(0x800a7ef0) char *DAT_800a7ef0[11] = {
	"BALANCED SLIDE",
	"INTO BACKSLIDE",
	"FS BOARDSLIDE",
	"BS BOARDSLIDE",
	"5-0 GRIND",
	"NOSEGRIND",
	"50 50 GRIND",
	"SMITH GRIND",
	"CROOKED GRIND",
	"NOSESLIDE",
	"TAILSLIDE",
};

/* CVector (THPS2 demo: CVector) */
PSX_DATA(0x800a7f1c) int WorldUp[3];

/* THPS2 demo: short[14] */
PSX_DATA(0x800a8228) short HeadBloodAtFrame[12] = {
	19, -1, 17, 17, 17, 17, 17, 17, 17, 17, 17,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1704) int Cheat_L2KillsBaddies[1];

/* THPS2 demo: int */
PSX_DATA(0x800d172c) int Cheat_Fire[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1770) int XFrames[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1774) int TimeScale[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1778) int TimeScaleSquared[1];

/* THPS2 demo: int */
PSX_DATA(0x800d179c) int GHorsePlayer[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1868) int BitCount[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1924) int Cheat_StatsOverride[2] = {
	0, 1,
};

PSX_DATA(0x800d1938) int D_800d1938[1] = {
	49,
};

PSX_DATA(0x800d1950) int DAT_800d1950[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1974) int Face_Air[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1978) int Face_Skateable[1];

/* THPS2 demo: int */
PSX_DATA(0x800d197c) int Face_WallRide[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1980) int Face_Visible[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1984) int Face_Terrain[1];

/* THPS2 demo: int */
PSX_DATA(0x800d20d4) int BulletCollisionCheck[1];
