/* StateTransition__6CBruce13EPhysicsStatei (0x8002f980) */
#include "names.h"
typedef struct {
	int x, y, z;
} V;

typedef struct {
	char p0[0x134];
	int f134;
	char p138[0x146 - 0x138];
	unsigned short f146;
	char p148[0x230 - 0x148];
	int f230;
	char p234[0x260 - 0x234];
	int f260, f264;
	char p268[0x26c - 0x268];
	int f26c, f270, f274;
	char p278[0x27c - 0x278];
	int f27c, f280;
	char p284[0x734 - 0x284];
	int f734;
	char p738[0x7d8 - 0x738];
	V w0;
	V w1;
	V w2;
	V w3;
} B;

extern void CBruce_Trick_Trick(B *, int, void *, int, int);
extern int Mips_FixMul(int, int);
extern void CBruce_RotMatrixFromFRUVectors(B *);
extern V __mi__C7CVector(const V *);

extern char s_WALL_RIDE[];

void CBruce_StateTransition(B *o, int n, int line) {
	V a, r, r2;

	if (o->f734 == 4 && n == 1) {
		o->f264 = 0;
		o->f260 = 0;
		o->f280 = 0;
		o->f27c = 0;
		o->f270 = 0;
		o->f274 = 0;
		o->f26c = 0;
	}

	if (o->f734 != n) {
		if (n == 1 && o->f734 != 2)
			o->f230 = o->f230 + 1;

		if (n == 2 && o->f734 != 1)
			o->f230 = o->f230 + 1;
	}

	if (o->f734 != 2) {
		if (n == 2) {
			o->w3 = o->w0;
			o->w3.x = -Mips_FixMul(o->w3.x, 13000);
			o->w3.y = Mips_FixMul(o->w3.y, 13000);
			o->w3.z = -Mips_FixMul(o->w3.z, 13000);
		}
	}

	if (o->f734 == 5 && n == 1) {
		CBruce_Trick_Trick(o, 64, s_WALL_RIDE, 500, 0);

		if (o->f146 == 35) {
			if (!(o->f134 & 2)) {
				a = o->w1;
				o->w1 = o->w2;
				r2 = __mi__C7CVector(&a);
				o->w2 = r2;
			} else
				goto blk;
		} else if (o->f134 & 2) {
			a = o->w1;
			o->w1 = o->w2;
			r = __mi__C7CVector(&a);
			o->w2 = r;
		} else {
		blk:
			a = o->w1;
			r = __mi__C7CVector(&o->w2);
			o->w1 = r;
			o->w2 = a;
		}
	}

	CBruce_RotMatrixFromFRUVectors(o);

	o->f734 = n;
}
