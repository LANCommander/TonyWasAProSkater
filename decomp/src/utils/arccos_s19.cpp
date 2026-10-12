/* Utils_ArcCos__Fi (0x800480f8) */
#include "game/trig.h"

extern SSinCos rcossin_tbl[2];

int Utils_ArcCos(int a) {
	int flag;
	int d;
	int t;

	flag = 0;

	if (a < 0) {
		a = -a;
		flag = 1;
	}

	d = 256;
	t = 512;

	for (;;) {
		SSinCos *p = &rcossin_tbl[t];
		int c = p->c;

		if (a < c) {
			t += d;
		} else if (c == a)
			break;
		else
			t -= d;

		d >>= 1;

		if (d == 0)
			break;
	}

	if (flag)
		return 2048 - t;

	return t;
}
