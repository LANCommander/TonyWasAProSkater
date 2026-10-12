/* Options_RemapControllers__Fv (0x80010518) */
/* C++ (CAREER.CPP). Demo twin. Points each pad's pTriangle/pSquare/pCircle/pX at the button the career options map it to. */
#include "game/career.h"
#include "game/pad.h"

extern SControl Pad[2];
extern SCareer GCareer;

void Options_RemapControllers() {
	int padNum;

	for (padNum = 0; padNum < 2; padNum++) {
		SButton *b = &Pad[padNum].Triangle;
		int *f = GCareer.Options.ControllerFunction[padNum];
		int i;

		for (i = 0; i < 4; i++) {
			switch (f[i]) {
				case 0:
					Pad[padNum].pTriangle = b + i;
					break;
				case 1:
					Pad[padNum].pSquare = b + i;
					break;
				case 2:
					Pad[padNum].pCircle = b + i;
					break;
				case 3:
					Pad[padNum].pX = b + i;
					break;
			}
		}
	}
}
