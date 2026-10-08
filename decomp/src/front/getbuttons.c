/* Front_GetButtons__FRiN30 (0x80019e94) */
#include "names.h"
typedef struct {
	unsigned char b[0x16c];
} SControl;

extern SControl Pad[];
extern int Cheat_ScreenShots, GameFrozen;

int GStart1, GStart2;

void Front_GetButtons(int *a, int *b, int *c, int *d) {
	int v, w, e;

	if (Cheat_ScreenShots == 0 || GameFrozen == 0) {
		v = 0;

		if (Pad[0].b[0xe1] || Pad[1].b[0xe1])
			v = 1;

		*d = v;
		e = Pad[0].b[0xe1];
		GStart1 = e;

		if (!e)
			GStart2 = Pad[1].b[0xe1];
		else
			GStart2 = 0;

		Pad[0].b[0xe1] = 0;
		Pad[1].b[0xe1] = 0;
	} else {
		*d = 0;
	}

	v = 0;

	if (Pad[0].b[0x31] || Pad[1].b[0x31] || *d)
		v = 1;

	*a = v;
	Pad[0].b[0x31] = 0;
	Pad[1].b[0x31] = 0;
	w = 0;

	if (Pad[0].b[1] || Pad[1].b[1])
		w = 1;

	*b = w;
	Pad[0].b[1] = 0;
	Pad[1].b[1] = 0;
	*c = *b | *a;

	if (Pad[0].b[0x11]) {
		Pad[0].b[0x11] = 0;
		*c = 1;
	}

	if (Pad[0].b[0x21]) {
		Pad[0].b[0x21] = 0;
		*c = 1;
	}

	if (Pad[0].b[0xf1]) {
		Pad[0].b[0xf1] = 0;
		*c = 1;
	}

	if (Pad[0].b[0x41]) {
		Pad[0].b[0x41] = 0;
		*c = 1;
	}

	if (Pad[0].b[0x51]) {
		Pad[0].b[0x51] = 0;
		*c = 1;
	}

	if (Pad[0].b[0x61]) {
		Pad[0].b[0x61] = 0;
		*c = 1;
	}

	if (Pad[0].b[0x71]) {
		Pad[0].b[0x71] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x11]) {
		Pad[1].b[0x11] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x21]) {
		Pad[1].b[0x21] = 0;
		*c = 1;
	}

	if (Pad[1].b[0xf1]) {
		Pad[1].b[0xf1] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x41]) {
		Pad[1].b[0x41] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x51]) {
		Pad[1].b[0x51] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x61]) {
		Pad[1].b[0x61] = 0;
		*c = 1;
	}

	if (Pad[1].b[0x71]) {
		Pad[1].b[0x71] = 0;
		*c = 1;
	}

	if (*a)
		*b = 0;
}
