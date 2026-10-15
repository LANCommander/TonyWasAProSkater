/* Decompiled data: the retail executable's initialised globals that src/campt/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: int[70] */
PSX_DATA(0x800be22c) int CamModes[70] = {
	1330, 1360, 1310, 1340, 1330, 1360, 1310, 2048,
};

/* THPS2 demo: short[70] */
PSX_DATA(0x800be344) short CamPtNode[70];

/* THPS2 demo: int */
PSX_DATA(0x800d1b6c) int CamPtIndex[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2134) int LastDpqMax[1];

/* THPS2 demo: int */
PSX_DATA(0x800d2138) int LastDpqMin[1];
