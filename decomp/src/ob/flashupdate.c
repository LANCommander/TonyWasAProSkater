/* FlashUpdate__5CItem (0x80043728) */
#include "names.h"

typedef struct {
	unsigned short flags;
	char p0[0x1e];
	unsigned c;
	char p1[0x1c];
	unsigned x34;
	char p2[0xc];
	unsigned short timer;
	short dr, dg, db;
} Item;

typedef struct {
	char p[0x20];
	unsigned c;
	char p1[0x10];
	unsigned x34;
	char p2[0x8];
	unsigned short timer;
	short dr, dg, db;
} Item2;

void CItem_FlashUpdate(Item2 *t) {
	unsigned short tm = t->timer;

	if (tm) {
		tm--;
		t->timer = tm;

		{
			unsigned c = t->c;
			int dr = t->dr, dg = t->dg, db = t->db;
			unsigned r = (c & 255) + dr;
			unsigned g = ((c >> 8) & 255) + dg;
			unsigned b = ((c >> 16) & 255) + db;

			t->c = ((b & 255) << 16) | ((g & 255) << 8) | (r & 255);
		}

		*(unsigned short *)t |= 0x400;

		if (!tm) {
			if (t->x34 & 0x02000000)
				*(unsigned short *)t |= 0x400;
			else
				*(unsigned short *)t &= ~0x400;

			t->c = t->x34;
			t->x34 = 0;
		}
	}
}
