/* FUN_800440ec (0x800440ec) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short x, y, z;
} S6;

struct VT {
	char pad[0x18];
	short offA;
	short pA;
	void (*fnA)(char *);
	short offB;
	short pB;
	void (*fnB)(char *);
};

typedef struct {
	unsigned short fl;
	char p0[2];
	V pos;
	S6 ang;
	char p1[0x48 - 0x16];
	struct VT *vt;
	char p2[0xed - 0x4c];
	unsigned char n;
	unsigned char k;
	char p3;
	V old;
	char p4[0x108 - 0xfc];
	V v108;
	V v114;
	S6 a120;
	S6 a126;
	S6 a12c;
	char p5[0x144 - 0x132];
	unsigned short w144;
	unsigned char b146;
	char p6[2];
	unsigned char b149;
	char p7[0x154 - 0x14a];
	unsigned short w154;
	unsigned short w156;
	unsigned char b158;
	unsigned char b159;
} B;

void FUN_80043f70(B *);
V __mi__FRC7CVectorT0(const V *, const V *);
V __dv__FRC7CVectorRCi(const V *, const int *);
V __mi__FRC8CSVectorT0(const S6 *, const S6 *);
int __adv__8CSVectorRCi(S6 *, const int *);
void __apl__7CVectorRC7CVector(V *, const V *);
void __apl__8CSVectorRC8CSVector(S6 *, const S6 *);
void CSVector_Mask(S6 *);

void FUN_800440ec(B *p) {
	V t;
	V r1;
	int n1, n2;
	S6 *s0;

	if (p->fl & 2) {
		FUN_80043f70(p);
		p->vt->fnB((char *)p + p->vt->offB);

		if (p->n < 2) {
			p->w156 = p->w144;
			p->vt->fnA((char *)p + p->vt->offA);

			p->w154 = p->w144;
			p->b158 = p->b146;
			p->b159 = p->b149;
			return;
		}
	} else {
		p->vt->fnB((char *)p + p->vt->offB);

		if (p->n < 2) {
			p->vt->fnA((char *)p + p->vt->offA);
			return;
		}
	}

	p->k++;

	if (p->k >= p->n) {
		p->k = 0;
		p->pos = *(V *)&p->v108;
		p->old = p->pos;
		p->ang = p->a126;
		p->a120 = p->ang;

		if (p->fl & 2) {
			p->w156 = p->w144;
			p->vt->fnA((char *)p + p->vt->offA);

			p->w154 = p->w144;
			p->b158 = p->b146;
			p->b159 = p->b149;
		} else {
			p->vt->fnA((char *)p + p->vt->offA);
		}

		p->v108 = p->pos;
		p->pos = p->old;
		r1 = __mi__FRC7CVectorT0(&p->v108, &p->old);
		n1 = p->n;
		t = __dv__FRC7CVectorRCi(&r1, &n1);
		p->v114 = t;
		p->a126 = p->ang;
		p->ang = p->a120;
		t = __mi__FRC8CSVectorT0(&p->a126, &p->a120);
		p->a12c = *(S6 *)&t;

		if (p->a12c.x < -2048)
			p->a12c.x += 4096;

		if (p->a12c.x > 2048)
			p->a12c.x -= 4096;

		if (p->a12c.y < -2048)
			p->a12c.y += 4096;

		if (p->a12c.y > 2048)
			p->a12c.y -= 4096;

		if (p->a12c.z < -2048)
			p->a12c.z += 4096;

		if (p->a12c.z > 2048)
			p->a12c.z -= 4096;

		n2 = p->n;
		__adv__8CSVectorRCi(&p->a12c, &n2);
		return;
	}

	__apl__7CVectorRC7CVector(&p->pos, &p->v114);

	s0 = &p->ang;
	__apl__8CSVectorRC8CSVector(s0, &p->a12c);
	CSVector_Mask(s0);
}
