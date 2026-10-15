/* FUN_80060738 (0x80060738) */
#include "names.h"

typedef struct {
	char *v[7];
} T;

typedef struct {
	int v[7];
} K;

struct VerticalMenu;

extern T DAT_800a1f34;
extern unsigned char DAT_800a6858[];

int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);

int FUN_80060738(int arg, int *p) {
	int flag = 0;
	int i;
	int r;
	T a;
	K b;

	for (i = 0; i < 11; i++) {
		if (DAT_800a6858[i * 14] & 224)
			flag = 1;
	}

	a = DAT_800a1f34;
	b.v[0] = 0;
	b.v[1] = p[5] < 3;
	b.v[2] = p[6] < 3;
	b.v[3] = p[7] < 3;
	b.v[4] = p[8] < 3;
	b.v[5] = p[9] < 3;
	b.v[6] = flag == 0;
	r = Front_RunMenu(a.v, b.v, 7, 25200, &arg, 0, 0, 0, 0);

	if (r == 25201)
		r = -1;

	return r;
}
