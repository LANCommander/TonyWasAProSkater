/* Decompiled data: the retail executable's initialised globals that src/card/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: char[28] */
PSX_DATA(0x8009e130) unsigned char CARD_GTITLE[28] = {
	0x82, 0x73, 0x82, 0x67, 0x82, 0x6f, 0x82, 0x72, 0x81, 0x40, 0x82, 0x62, 0x82, 0x60, 0x82, 0x71,
	0x82, 0x64, 0x82, 0x64, 0x82, 0x71, 0x81, 0x40, 0x81, 0x5b, 0x81, 0x40,
};

/* THPS2 demo: char[28] */
PSX_DATA(0x8009e150) unsigned char CARD_VTITLE[28] = {
	0x82, 0x73, 0x82, 0x67, 0x82, 0x6f, 0x82, 0x72, 0x81, 0x40, 0x82, 0x75, 0x82, 0x68, 0x82, 0x63,
	0x82, 0x64, 0x82, 0x6e, 0x81, 0x40, 0x81, 0x5b, 0x81, 0x40,
};

/* THPS2 demo: char[25] */
PSX_DATA(0x800a5848) char CARD_FNAME[] = "bu00:BASLUS-00860THPSGxx";

/* THPS2 demo: char[25] */
PSX_DATA(0x800a5864) char CARD_VNAME[] = "bu00:BASLUS-00860THPSVxx";

/* ESaveAction (THPS2 demo: ESaveAction) */
PSX_DATA(0x800d1148) int GSaveAction[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1380) int CardStatus[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1384) int CardLoadFlag[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1390) int CardStatusTemp[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1394) int CardStatusMode[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1398) int TimeStamp[1];

/* THPS2 demo: ulong */
PSX_DATA(0x800d139c) unsigned int ev0[1];

/* THPS2 demo: ulong */
PSX_DATA(0x800d13a0) unsigned int ev1[1];

/* THPS2 demo: ulong */
PSX_DATA(0x800d13a4) unsigned int ev2[1];

/* THPS2 demo: ulong */
PSX_DATA(0x800d13a8) unsigned int ev3[1];

/* THPS2 demo: int */
PSX_DATA(0x800d13bc) int EVENT_TIME_OUT[1];

/* THPS2 demo: long */
PSX_DATA(0x800d13c0) int FILE_DSCRPTR[1];
