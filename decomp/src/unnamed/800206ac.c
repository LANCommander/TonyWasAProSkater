/* FUN_800206ac (0x800206ac) */
#include "names.h"
typedef struct {
	int x, y, z;
} V;

typedef struct {
	short v;
	short pad[3];
} TS;

struct VT {
	char pad[0x38];
	short off;
	short pad2;
	void (*fn)(char *);
};

typedef struct G {
	char pad[0x42];
	unsigned char b42;
	char pad2[0x50 - 0x43];
	unsigned cnt;
	char pad3[0x5e - 0x54];
	unsigned char b5e;
} G;

typedef struct P {
	unsigned short fl;
	char pad[2];
	V v;
	char pad2[0x3a - 0x10];
	unsigned short type;
	char pad3[0x48 - 0x3c];
	struct VT *vt;
	char pad4[0xe8 - 0x4c];
	unsigned e8;
	char pad5[0x134 - 0xec];
	G *g;
	unsigned short w138;
	unsigned short w13a;
	char pad6[0x13d - 0x13c];
	unsigned char b13d;
} P;

typedef struct Q {
	char pad[0x1c];
	V *f1c;
} Q;

extern int GNumberOfPlayers;
extern Q *MechList;
extern int TTime;
extern char rcossin_tbl[];

void CPowerUp_DoPhysics(P *);
void CPowerUp_CreateBit(P *);
void FUN_800201c8(P *);
G *__nw__4CBitUi(int);
G *__5CGlowUiUi(G *, int, int);
void CGlow_SetCentreRGB(G *, unsigned char, unsigned char, unsigned char);
void CGlow_SetFringeRGB(G *, unsigned, unsigned char, unsigned char, unsigned char);
void CGlow_SetFringeWidth(G *, unsigned, unsigned);
void CBit_SetPos(G *, V *);
void CGlow_SetRGB(G *, unsigned char, unsigned char, unsigned char);
void CGlow_SetRadialRGB(G *, int, unsigned char, unsigned char, unsigned char);
void CGlow_SetRadius(G *, int);
unsigned Utils_CrapDist(V *, V *);

void FUN_800206ac(P *p) {
	unsigned v;
	unsigned i;
	char *tb;
	int t, idx, m;
	G *g;
	G *lg;
	V *s3;
	int s0;

	FUN_800201c8(p);

	if (GNumberOfPlayers == 1) {
		if (p->fl & 1)
			return;

		v = p->e8 < 8001;
		goto test;
	} else if ((p->fl & 0x41) == 0x41) {
		return;
	} else if (p->e8 >= 8001) {
		v = Utils_CrapDist((V *)((char *)MechList->f1c + 4), &p->v) < 8001;
	test:
		if (!v) {
			p->vt->fn((char *)p + p->vt->off);
			return;
		}
	}

	CPowerUp_DoPhysics(p);

	s3 = &p->v;

	if (p->b13d == 0)
		CPowerUp_CreateBit(p);

	if (p->g == 0) {
		g = __nw__4CBitUi(104);

		if (g)
			g = __5CGlowUiUi(g, 4, 2);

		p->g = g;
		g->b5e = 1;
		CGlow_SetCentreRGB(p->g, 0, 0, 0);
		CGlow_SetRGB(p->g, 0, 0, 0);
		CGlow_SetFringeRGB(p->g, 1, 0, 0, 0);
		CGlow_SetRadius(p->g, 0);

		p->g->b42 = 1;

		switch (p->type) {
			case 18:
				i = 0;
				lg = p->g;

				if (lg->cnt != 0) {
					do {
						if (i & 1)
							CGlow_SetRadialRGB(lg, i, 96, 96, 0);
						else
							CGlow_SetRadialRGB(p->g, i, 64, 64, 64);

						i++;
						lg = p->g;
					} while (i < lg->cnt);
				}

				CGlow_SetRGB(p->g, 32, 0, 0);
				CGlow_SetFringeRGB(p->g, 1, 0, 0, 0);

				p->w138 = 10;
				p->w13a = 3;
				break;
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
				i = 0;
				lg = p->g;

				if (lg->cnt != 0) {
					do {
						if (i & 1)
							CGlow_SetRadialRGB(lg, i, 0, 0, 96);
						else
							CGlow_SetRadialRGB(p->g, i, 0, 0, 0);

						i++;
						lg = p->g;
					} while (i < lg->cnt);
				}

				CGlow_SetRGB(p->g, 0, 0, 0);
				CGlow_SetFringeRGB(p->g, 1, 0, 0, 0);

				p->w138 = 60;
				p->w13a = 10;
				break;
			case 16:
				i = 0;
				lg = p->g;

				if (lg->cnt != 0) {
					do {
						if (i & 1)
							CGlow_SetRadialRGB(lg, i, 96, 96, 0);
						else
							CGlow_SetRadialRGB(p->g, i, 64, 64, 64);

						i++;
						lg = p->g;
					} while (i < lg->cnt);
				}

				CGlow_SetRGB(p->g, 0, 0, 0);
				CGlow_SetFringeRGB(p->g, 1, 0, 0, 0);

				p->w138 = 60;
				p->w13a = 10;
				break;
			case 21:
				CGlow_SetFringeRGB(p->g, 0, 48, 48, 0);

				p->w138 = 60;
				p->w13a = 10;
				break;
			case 22:
				CGlow_SetFringeRGB(p->g, 0, 64, 32, 8);

				p->w138 = 60;
				p->w13a = 10;
				break;
			case 23:
				CGlow_SetFringeRGB(p->g, 0, 48, 0, 0);

				p->w138 = 60;
				p->w13a = 10;
				break;
		}
	}

	CBit_SetPos(p->g, s3);

	s0 = p->w138 + (p->w13a * ((short *)rcossin_tbl)[((TTime * 600) & 0x3ff8) >> 1]) / 4096;
	CGlow_SetFringeWidth(p->g, 0, s0);
	CGlow_SetFringeWidth(p->g, 1, s0);
}
