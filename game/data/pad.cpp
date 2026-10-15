/* Decompiled data: the retail executable's initialised globals that src/pad/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* ControllerBuffer[2] (THPS2 demo: ControllerBuffer[2]) */
PSX_DATA(0x800c0d20) int controllers[17];

/* ControllerData[8] (THPS2 demo: ControllerData[8]) */
PSX_DATA(0x800c0d64) int theControllers[16];

/* THPS2 demo: int */
PSX_DATA(0x800d1f68) int Pad_IdleTime[1];

/* THPS2 demo: uchar[6] */
PSX_DATA(0x800d1f6c) unsigned char align[8] = {
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1f74) int ControlSystemInitialised[1];
