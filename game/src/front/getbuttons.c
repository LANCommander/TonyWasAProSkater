/* Front_GetButtons__FRiN30 (0x80019e94) */
/* The front end's buttons this frame, each consumed: a = activate (menu accept, or Start on the
   PS1), b = go back (cleared when a is set), c = any button, d = Start/pause (and GStart1/2 say
   which pad pressed it). */
#include "names.h"
#include "game/padbuttons.h"

typedef struct {
	unsigned char b[PAD_SIZE];
} SControl;

extern SControl Pad[];
extern int Cheat_ScreenShots, GameFrozen;

int GStart1, GStart2;

#define TRIG(pad, button) Pad[pad].b[(button) + PAD_TRIGGERED]

void Front_GetButtons(int *a, int *b, int *c, int *d) {
	int v, w, e;

	if (Cheat_ScreenShots == 0 || GameFrozen == 0) {
		v = 0;

		if (TRIG(0, PAD_PAUSE) || TRIG(1, PAD_PAUSE))
			v = 1;

		*d = v;
		e = TRIG(0, PAD_PAUSE);
		GStart1 = e;

		if (!e)
			GStart2 = TRIG(1, PAD_PAUSE);
		else
			GStart2 = 0;

		TRIG(0, PAD_PAUSE) = 0;
		TRIG(1, PAD_PAUSE) = 0;
	} else {
		*d = 0;
	}

	v = 0;

	/* Pause is bound on its own on PC (often to the same key as Back), so it doesn't activate */
	if (TRIG(0, PAD_MENU_ACCEPT) || TRIG(1, PAD_MENU_ACCEPT))
		v = 1;

	*a = v;
	TRIG(0, PAD_MENU_ACCEPT) = 0;
	TRIG(1, PAD_MENU_ACCEPT) = 0;
	w = 0;

	if (TRIG(0, PAD_MENU_BACK) || TRIG(1, PAD_MENU_BACK))
		w = 1;

	*b = w;
	TRIG(0, PAD_MENU_BACK) = 0;
	TRIG(1, PAD_MENU_BACK) = 0;
	*c = *b | *a;

	if (TRIG(0, PAD_FLIP)) {
		TRIG(0, PAD_FLIP) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_GRAB)) {
		TRIG(0, PAD_GRAB) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_CAMERA)) {
		TRIG(0, PAD_CAMERA) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_SPIN_LEFT)) {
		TRIG(0, PAD_SPIN_LEFT) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_TAP_SPIN_LEFT)) {
		TRIG(0, PAD_TAP_SPIN_LEFT) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_SPIN_RIGHT)) {
		TRIG(0, PAD_SPIN_RIGHT) = 0;
		*c = 1;
	}

	if (TRIG(0, PAD_TAP_SPIN_RIGHT)) {
		TRIG(0, PAD_TAP_SPIN_RIGHT) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_FLIP)) {
		TRIG(1, PAD_FLIP) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_GRAB)) {
		TRIG(1, PAD_GRAB) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_CAMERA)) {
		TRIG(1, PAD_CAMERA) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_SPIN_LEFT)) {
		TRIG(1, PAD_SPIN_LEFT) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_TAP_SPIN_LEFT)) {
		TRIG(1, PAD_TAP_SPIN_LEFT) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_SPIN_RIGHT)) {
		TRIG(1, PAD_SPIN_RIGHT) = 0;
		*c = 1;
	}

	if (TRIG(1, PAD_TAP_SPIN_RIGHT)) {
		TRIG(1, PAD_TAP_SPIN_RIGHT) = 0;
		*c = 1;
	}

	if (*a)
		*b = 0;
}
