/* GetUpDown__Fv (0x80011240) */
#include "game/padbuttons.h"
extern unsigned char Pad[];
extern int LeftRightAutoRepeatTimer;

int GetUpDown(void) {
	int r = 0;

	if (Pad[PAD_MENU_UP + PAD_PRESSED] || Pad[PAD_SIZE + PAD_MENU_UP + PAD_PRESSED] || Pad[PAD_MENU_DOWN + PAD_PRESSED] || Pad[PAD_SIZE + PAD_MENU_DOWN + PAD_PRESSED]) {
		if (LeftRightAutoRepeatTimer == 0 || (LeftRightAutoRepeatTimer >= 16 && (LeftRightAutoRepeatTimer & 3) == 0)) {
			if (Pad[PAD_MENU_DOWN + PAD_PRESSED] || Pad[PAD_SIZE + PAD_MENU_DOWN + PAD_PRESSED])
				r = 1;

			if (Pad[PAD_MENU_UP + PAD_PRESSED] || Pad[PAD_SIZE + PAD_MENU_UP + PAD_PRESSED])
				r = -1;
		}

		LeftRightAutoRepeatTimer++;
	} else {
		LeftRightAutoRepeatTimer = 0;
	}

	return r;
}
