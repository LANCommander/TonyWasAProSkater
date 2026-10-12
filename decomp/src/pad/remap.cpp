/* Pad_Remap__Fv (0x80071588) */
/* C++ (PAD.CPP). Demo twin Pad_Remap (its extra swap block is gone in THPS1): copies each pad's
   face buttons out to the slots their remap pointers name. Inverse of Pad_Restore. */
#include "game/pad.h"

extern SControl Pad[];

void Pad_Remap(void) {
#ifndef PC_PORT /* on PC the bindings already say what every key does: no remap */
	SButton TempButtonTriangle;
	SButton TempButtonX;
	SButton TempButtonCircle;
	SButton TempButtonSquare;

	{
		int i;

		for (i = 0; i < 2; i++) {
			TempButtonTriangle = Pad[i].Grind;
			TempButtonCircle = Pad[i].Grab;
			TempButtonSquare = Pad[i].Flip;
			TempButtonX = Pad[i].Ollie;
			*Pad[i].pGrind = TempButtonTriangle;
			*Pad[i].pGrab = TempButtonCircle;
			*Pad[i].pFlip = TempButtonSquare;
			*Pad[i].pOllie = TempButtonX;
		}
	}
#endif
}
