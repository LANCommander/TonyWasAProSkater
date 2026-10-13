/* FUN_80035f30 (0x80035f30) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short x, y, z;
} S6;

typedef struct {
	char p0[0x6c];
	V vel;
	char p1[0x150 - 0x78];
	int f150;
	char p2[0x24c - 0x154];
	int f24c;
	char p3[0x358 - 0x250];
	int f358;
	char p4[0x734 - 0x35c];
	int f734;
	char p5[0x7d8 - 0x738];
	V w1;
	char p6[0x7f0 - 0x7e4];
	V w2;
} B;

void CBruce_Debug_RunAnim(B *, int, int);
void CBruce_Bail(B *, int);
int Mips_CVecDotProd(V *, V *);
void CBruce_RotMatrixFromFRUVectors(B *);
V __mi__C7CVector(const V *);
V __ml__FRC7CVectorRCi(const V *, const int *);

void FUN_80035f30(B *p, S6 s) {
	V v;
	V t;
	V r;
	V r2;
	int n;

	if (p->f358 == 0) {
		if (p->f734 != 2) {
			v.x = s.x;
			v.y = s.y;
			v.z = s.z;

			if (Mips_CVecDotProd(&p->w2, &v) < 2000 && p->w2.y >= -1999) {
				CBruce_Bail(p, 0);

				p->f150 = 0x10000;
				CBruce_Debug_RunAnim(p, 56, 4860);

				p->f24c = 9;
				t = p->w1;
				r = __mi__C7CVector(&p->w2);
				p->w1 = r;
				p->w2 = t;
				CBruce_RotMatrixFromFRUVectors(p);

				r2 = __mi__C7CVector(&p->w1);
				n = 10;
				r = __ml__FRC7CVectorRCi(&r2, &n);
				p->vel = r;
				p->vel.y = 0;
			}
		}
	}
}
