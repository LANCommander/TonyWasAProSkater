/* Video_Start__FP6CBruce10EVideoModeii (0x80069684) */
#include "names.h"

typedef struct {
	int f0;
	int *f4;
	int f8;
	char p0[0x10 - 0xc];
	int f10;
	int f14;
	int f18;
	int f1c;
	int f20;
	int f24;
	char p1[0x38 - 0x28];
	int f38;
	int f3c;
	int f40;
	int f44;
	char p2[0x4c - 0x48];
	int f4c;
	char p3[0x54 - 0x50];
	int f54[8];
	char p4[4];
	int f78;
	int f7c;
	int f80;
	short s84, s86, s88;
	char p5[0x8c - 0x8a];
	short s8c, s8e, s90;
	char p6[0x94 - 0x92];
	int f94, f98, f9c, fa0, fa4, fa8, fac;
	char p7[0xb4 - 0xb0];
	int fb4, fb8, fbc;
} S;

typedef struct {
	char p[0x144];
	short s144, s146;
	char q[0x724 - 0x148];
	int f724;
} B;

extern S GVInfo1[];
extern S GVInfo2[];
extern int MechList;

int HLStartCalled;

extern int Xblanks;

void FUN_80068d88(void);
void FUN_80068ed4(void);
void FUN_80068f2c(void);
void CBruce_BoardOn(B *);

void Video_Start(B *a, int b, int c, int d) {
	S *s;
	int i, j, x, y;
	int *q;
	int *t;
	int g;

	if ((int)a == MechList)
		s = GVInfo1;
	else
		s = GVInfo2;

	HLStartCalled = 0;
	FUN_80068d88();
	FUN_80068ed4();
	FUN_80068f2c();

	s->f0 = b;

	if (b == 1) {
		s->f1c = 0;
		s->f18 = 32;

		for (j = 0; j < 5; j++)
			s->f54[j * 2] = 0;

		q = s->f4;
		s->f4c = 0;

		for (i = 0; i < 6144; i++) {
			*q = 0;
			q++;
		}
	} else {
		s->f18 = 0;
	}

	x = s->f8;
	y = s->f1c;
	s->f14 = 0;
	s->f24 = -1;
	s->f44 = -2;
	s->f38 = 0;
	s->f40 = 0;
	s->f3c = -1;
	s->f10 = x;
	s->f20 = y;

	if (d != 0x7fffffff)
		s->f20 = d - c + 1;

	g = Xblanks;
	s->f9c = 0;
	s->f98 = 0;
	s->f94 = 0;
	s->fa8 = 0;
	s->fa4 = 0;
	s->fa0 = 0;
	s->s88 = 0;
	s->s86 = 0;
	s->s84 = 0;
	s->s90 = 0;
	s->s8e = 0;
	s->s8c = 0;
	s->f78 = 0;
	s->f7c = 0;
	s->f80 = 0;
	s->fac = 0;
	s->fbc = 0;
	s->fb4 = g;
	s->fb8 = g;
	a->s146 = 0;
	a->s144 = 0;
	a->f724 = 0;
	CBruce_BoardOn(a);
}
