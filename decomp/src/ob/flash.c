/* Flash__5CItemiUcUcUc (0x800435e8) */
#include "names.h"

typedef struct {
	unsigned short flags;
	char _2[0x1e];
	unsigned col;
	char _24[0x10];
	unsigned f34;
	char _38[8];
	unsigned short cnt;
	short s0, s1, s2;
} Item;

void CItem_Flash(Item *t, int n, unsigned char r0, unsigned char g0, unsigned char b0) {
	int r = r0, g = g0, b = b0;

	if (n == 0) {
		t->cnt = 0;
	} else if (n < 0) {
		t->cnt = -n;
		t->col = 0x808080;
		t->flags |= 0x400;
		t->s0 = (r - 128) / t->cnt;
		t->s1 = (g - 128) / t->cnt;
		t->s2 = (b - 128) / t->cnt;
		return;
	} else {
		t->s0 = (128 - r) / n;
		t->s1 = (128 - g) / n;
		t->s2 = (128 - b) / n;
		t->cnt = n + 1;

		if (!(t->f34 & 0x1000000)) {
			t->f34 = (t->col & 0xffffff) | 0x1000000;

			if (t->flags & 0x400)
				t->f34 |= 0x2000000;
		}
	}

	t->col = (b << 16) | (g << 8) | r;
	t->flags |= 0x400;
}
