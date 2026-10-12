/* Pad_Restore__Fv (0x8007171c) */
#include "game/pad.h"

extern SControl Pad[];

void Pad_Restore(void) {
	SButton TempButtonTriangle;
	SButton TempButtonX;
	SButton TempButtonCircle;
	SButton TempButtonSquare;

	{
		int i;

		for (i = 0; i < 2; i++) {
			TempButtonTriangle = *Pad[i].pTriangle;
			TempButtonCircle = *Pad[i].pCircle;
			TempButtonSquare = *Pad[i].pSquare;
			TempButtonX = *Pad[i].pX;
			Pad[i].Triangle = TempButtonTriangle;
			Pad[i].Circle = TempButtonCircle;
			Pad[i].Square = TempButtonSquare;
			Pad[i].X = TempButtonX;
		}
	}
}
