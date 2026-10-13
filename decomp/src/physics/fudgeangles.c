/* FudgeAngles__FP6CBruce (0x80044b90) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	char _0[0x10];
	short a10;
	unsigned short a12;
	short a14;
	char _16[0x78c - 0x16];
	short m[9];
	char _79e[0x7d8 - 0x7a0 + 2];
	int r[3];
	char _7e4[0x7f0 - 0x7e4];
	int u[3];
} CBruce;

void Utils_RotateY(V *, V *, int);

void FudgeAngles(CBruce *t) {
	V a, r;
	int ang;

	t->a14 = 0;
	t->a10 = 0;
	a.x = -4096;
	a.y = 0;
	a.z = 0;
	ang = (t->a12 + 0x800) & 0xfff;
	t->a12 = ang;
	Utils_RotateY(&r, &a, ang);

	a.x = 0;
	a.y = 0;
	a.z = -4096;
	t->m[0] = r.x;
	t->m[3] = r.y;
	t->m[6] = r.z;
	Utils_RotateY(&r, &a, (short)t->a12);

	t->m[4] = -4096;
	t->u[1] = -4096;
	t->m[1] = 0;
	t->u[0] = 0;
	t->m[7] = 0;
	t->u[2] = 0;
	t->m[2] = r.x;
	t->r[0] = (short)r.x;
	t->m[5] = r.y;
	t->r[1] = (short)r.y;
	t->m[8] = r.z;
	t->r[2] = (short)r.z;
}
