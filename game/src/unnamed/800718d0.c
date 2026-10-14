/* FUN_800718d0 (0x800718d0) */
/* Any button other than X and Start triggered on either pad (the loading screen takes it as X). */
#include "game/padbuttons.h"

extern unsigned char Pad[];

#define TRIG(i, button) Pad[(i) * PAD_SIZE + (button) + PAD_TRIGGERED]

int FUN_800718d0(void) {
	int i;

	for (i = 0; i < 2; i++) {
		if (TRIG(i, PAD_GRIND) || TRIG(i, PAD_FLIP) || TRIG(i, PAD_SPIN_LEFT) ||
		    TRIG(i, PAD_TAP_SPIN_LEFT) || TRIG(i, PAD_SPIN_RIGHT) || TRIG(i, PAD_TAP_SPIN_RIGHT) ||
		    TRIG(i, PAD_LEFT) || TRIG(i, PAD_RIGHT) || TRIG(i, PAD_UP) || TRIG(i, PAD_DOWN) ||
		    TRIG(i, PAD_GRAB) || TRIG(i, PAD_CAMERA)
		    || TRIG(i, PAD_MENU_BACK) || TRIG(i, PAD_MENU_UP) || TRIG(i, PAD_MENU_DOWN) ||
		    TRIG(i, PAD_MENU_LEFT) || TRIG(i, PAD_MENU_RIGHT)
		)
			return 1;
	}

	return i < 2;
}
