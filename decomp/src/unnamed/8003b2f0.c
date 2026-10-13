/* FUN_8003b2f0 (0x8003b2f0) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct B {
	char p0[4];
	V pos;
	char p1[0x6c - 0x10];
	V vel;
	char p2[0xf0 - 0x78];
	V old;
	char p3[0x1d4 - 0xfc];
	struct B *other;
	char p4[0x358 - 0x1d8];
	int f358;
} B;

int Mips_CVecDotProd(V *, V *);
int CVector_Length(V *);
void CVector_Normalize(V *);
V __mi__FRC7CVectorT0(const V *, const V *);
void CBruce_Bail(B *, int);

void FUN_8003b2f0(B *p) {
	V d;
	int pd;
	V c;
	int pc;
	int s0, s1, v;
	V *s3;
	B *a;

	if (p->f358 == 0) {
		if (p->other->f358 == 0) {
			d = __mi__FRC7CVectorT0(&p->other->pos, &p->pos);

			if (CVector_Length(&d) < 60) {
				c = __mi__FRC7CVectorT0(&p->other->pos, &p->old);
				d = c;
				CVector_Normalize(&d);

				s3 = &p->vel;
				s1 = Mips_CVecDotProd(s3, &d);
				v = Mips_CVecDotProd(&p->other->vel, &d);

				if (s1 > 0) {
					if (v > 0) {
						CBruce_Bail(p->other, 0);
					} else {
						s0 = CVector_Length(&p->other->vel);

						if (CVector_Length(s3) >= s0)
							CBruce_Bail(p->other, 0);
						else
							CBruce_Bail(p, 0);
					}
				} else if (v <= 0)
					CBruce_Bail(p, 0);
			}
		}
	}
}
