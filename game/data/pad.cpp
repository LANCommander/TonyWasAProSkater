/* Decompiled data: the retail executable's initialised globals that src/pad/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

/* ControllerBuffer[2] (THPS2 demo: ControllerBuffer[2]) */
int controllers[17];

/* ControllerData[8] (THPS2 demo: ControllerData[8]) */
int theControllers[16];

/* THPS2 demo: int */
int Pad_IdleTime[1];

/* THPS2 demo: uchar[6] */
unsigned char align[8] = {
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff,
};

/* THPS2 demo: int */
int ControlSystemInitialised[1];
