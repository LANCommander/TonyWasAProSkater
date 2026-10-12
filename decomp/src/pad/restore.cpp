/* Pad_Restore__Fv (0x8007171c) */
#include "game/pad.h"

extern SControl Pad[];

void Pad_Restore(void) {
#ifndef PC_PORT /* on PC the bindings already say what every key does: no remap */
	SButton TempButtonTriangle;
	SButton TempButtonX;
	SButton TempButtonCircle;
	SButton TempButtonSquare;

	{
		int i;

		for (i = 0; i < 2; i++) {
			TempButtonTriangle = *Pad[i].pGrind;
			TempButtonCircle = *Pad[i].pGrab;
			TempButtonSquare = *Pad[i].pFlip;
			TempButtonX = *Pad[i].pOllie;
			Pad[i].Grind = TempButtonTriangle;
			Pad[i].Grab = TempButtonCircle;
			Pad[i].Flip = TempButtonSquare;
			Pad[i].Ollie = TempButtonX;
		}
	}
#endif
}
