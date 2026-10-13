/* FUN_8001f794 (0x8001f794) */
#include "names.h"

struct VT {
	char p[8];
	short off;
	short p2;
	void (*fn)(char *, int);
};

typedef struct {
	char p[4];
	int f4;
	char q[4];
	int fc;
} C;

typedef struct {
	char p[0x42];
	unsigned char f42;
	char q[1];
	struct VT *f44;
} O;

extern C *MechList;
extern int UseCacheForBits;

char *__nw__4CBitUi(int, C *);
char *CWobblyGlow_ctor(char *, char *, int, int, int, int, int, int, int, int, int);

void FUN_8001f794(char *t) {
	int dx, dz, d;
	char *o;
	O *ob;
	C *cam;

	if (t[0x56]) {
		cam = MechList;
		dx = (*(int *)(t + 0x18) - cam->f4) >> 12;

		if (dx < 0)
			dx = -dx;

		dz = (*(int *)(t + 0x20) - cam->fc) >> 12;

		if (dz < 0)
			dz = -dz;

		if (dx < dz)
			d = dz + dx / 2;
		else
			d = dx + dz / 2;

		if (d < *(int *)(t + 0x5c)) {
			if (*(char **)(t + 0x60) == 0) {
				UseCacheForBits = 0;
				o = __nw__4CBitUi(176, cam);

				if (o)
					o = CWobblyGlow_ctor(o, t + 0x18, *(int *)(t + 0x48), *(int *)(t + 0x4c),
					                     *(int *)(t + 0x58), *(unsigned char *)(t + 0x50),
					                     *(unsigned char *)(t + 0x51), *(unsigned char *)(t + 0x52),
					                     *(unsigned char *)(t + 0x53), *(unsigned char *)(t + 0x54),
					                     *(unsigned char *)(t + 0x55));

				*(char **)(t + 0x60) = o;
				UseCacheForBits = 1;
				((O *)o)->f42 = 1;
			}

			*(short *)(*(char **)(t + 0x60) + 0x60) = (d * 5 * 2) / 16;
			return;
		}
	}

	ob = *(O **)(t + 0x60);

	if (ob) {
		ob->f44->fn((char *)ob + ob->f44->off, 3);

		*(char **)(t + 0x60) = 0;
	}
}
