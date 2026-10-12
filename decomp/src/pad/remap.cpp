/* Pad_Remap__Fv (0x80071588) */
/* C++ (PAD.CPP). Demo twin Pad_Remap (its extra swap block is gone in THPS1): copies each pad's
   face buttons out to the slots their remap pointers name. Inverse of Pad_Restore. */
#include "game/pad.h"

extern SControl Pad[];

void Pad_Remap(void) {
	SButton TempButtonTriangle;
	SButton TempButtonX;
	SButton TempButtonCircle;
	SButton TempButtonSquare;

	{
		int i;

		for (i = 0; i < 2; i++) {
			TempButtonTriangle = Pad[i].Triangle;
			TempButtonCircle = Pad[i].Circle;
			TempButtonSquare = Pad[i].Square;
			TempButtonX = Pad[i].X;
			*Pad[i].pTriangle = TempButtonTriangle;
			*Pad[i].pCircle = TempButtonCircle;
			*Pad[i].pSquare = TempButtonSquare;
			*Pad[i].pX = TempButtonX;
		}
	}
}
