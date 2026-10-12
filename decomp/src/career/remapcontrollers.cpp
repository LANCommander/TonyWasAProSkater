/* Options_RemapControllers__Fv (0x80010518) */
/* C++ (CAREER.CPP). Demo twin. Points each pad's pTriangle/pSquare/pCircle/pX at the button the career options map it to. */
#include "game/career.h"
#include "game/pad.h"

extern SControl Pad[2];
extern SCareer GCareer;

void Options_RemapControllers() {
#ifndef PC_PORT /* on PC the bindings already say what every key does: no remap */
	int padNum;

	for (padNum = 0; padNum < 2; padNum++) {
		SButton *b = &Pad[padNum].Grind;
		int *f = GCareer.Options.ControllerFunction[padNum];
		int i;

		for (i = 0; i < 4; i++) {
			switch (f[i]) {
				case 0:
					Pad[padNum].pGrind = b + i;
					break;
				case 1:
					Pad[padNum].pFlip = b + i;
					break;
				case 2:
					Pad[padNum].pGrab = b + i;
					break;
				case 3:
					Pad[padNum].pOllie = b + i;
					break;
			}
		}
	}
#endif
}
