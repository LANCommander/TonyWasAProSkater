/* Front_FreeBackground__Fv (0x80011570) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

struct E {
	struct O *p;
	int pad[3];
	int x;
};

int FreeSprite;

extern struct E BGCache[];

void Front_FreeBackground(void) {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		struct O *o = BGCache[i].p;

		if (o)
			o->vt->fn((char *)o + o->vt->off, 3);

		BGCache[i].x = 0;
	}

	FreeSprite = 0;
}
