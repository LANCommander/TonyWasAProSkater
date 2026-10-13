/* FUN_8002efa4 (0x8002efa4) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	int x, y, z;
} VA __attribute__((aligned(8)));

typedef struct {
	short m[3][3];
	int t[3];
} MATRIX;

typedef struct {
	short x, y, z, pad;
} SV;

typedef struct {
	char p0[4];
	V pos;
	char p1[0x720 - 0x10];
	int f720;
	unsigned short a724;
	char p2[0x78c - 0x726];
	MATRIX m1;
	MATRIX m2;
	V v7cc;
} B;

int TransMatrix(MATRIX *, VA *);
int ApplyMatrixLV(MATRIX *, VA *, VA *);
int MulMatrix(MATRIX *, MATRIX *);
int RotMatrixYXZ(SV *, MATRIX *);
void M3d_BuildTransform(B *);

void FUN_8002efa4(B *p) {
	MATRIX m;
	SV ang;
	VA a;
	VA b;
	VA c1;
	VA c2;

	p->m2 = p->m1;
	p->v7cc = p->pos;

	if (p->f720) {
		a.z = 0;
		a.y = 0;
		a.x = 0;
		TransMatrix(&p->m1, &a);
		TransMatrix(&m, &a);

		b.x = 0;
		b.y = 0x46000;
		b.z = 0;
		ApplyMatrixLV(&p->m1, &b, &c1);

		ang.z = 0;
		ang.y = 0;
		ang.x = p->a724 & 4095;
		RotMatrixYXZ(&ang, &m);
		MulMatrix(&p->m1, &m);
		ApplyMatrixLV(&p->m1, &b, &c2);

		p->pos.x = p->pos.x + c1.x - c2.x;
		p->pos.y = p->pos.y + c1.y - c2.y;
		p->pos.z = p->pos.z + c1.z - c2.z;
	}

	M3d_BuildTransform(p);
}
