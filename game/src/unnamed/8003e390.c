/* FUN_8003e390 (0x8003e390) */
#include "names.h"

typedef struct {
	char pad0[16];
	unsigned short uh10;
	unsigned short uh12;
	unsigned short uh14;
	char pad1[14];
	unsigned short uh24;
	unsigned short uh26;
	unsigned short uh28;
	char pad2[38];
	unsigned short uh50;
	char pad3[26];
	int i6c;
	char pad4[8];
	int i78;
	int i7c;
	int i80;
	char pad5[8];
	short h8c;
	char pad6[4];
	short h92;
	char pad7[264];
	unsigned char ub19c;
	char pad8[1];
	unsigned char ub19e;
	char pad9[1];
	unsigned char ub1a0;
	char pad10[3];
	unsigned short uh1a4;
	char pad11[6];
	int i1ac;
	int i1b0;
	char pad12[32];
	unsigned short uh1d4;
	char pad13[2];
	unsigned short uh1d8;
	char pad14[34];
	unsigned short uh1fc;
	char pad15[30];
	unsigned short uh21c;
	unsigned short uh21e;
	unsigned short uh220;
	unsigned short uh222;
	unsigned short uh224;
	unsigned short uh226;
	char pad16[20];
	int i23c;
	int i240;
	int i244;
	int i248;
} S;

typedef struct {
	short x, y, z;
} SV;

typedef struct {
	int x, y, z;
} V12;

typedef struct {
	int x, y, z;
} __attribute__((aligned(8))) V;

extern char DAT_800a7f28[];
extern int TTime;

void CPlatform_DoPhysics(S *);
void CPlatform_BouncyAI(S *);
void CBaddy_ParseScript(S *, int);
void SFX_ModifyPos(int, void *, int);
int CVector_Length(V *);
void Utils_CalcAim(SV *, void *, void *);
void Utils_GetVecFromMagDir(void *, int, SV *);
void Utils_TurnTowards(SV, SV *, SV *, SV, int);
void __apl__7CVectorRC7CVector(void *, void *);
void __mi__FRC7CVectorT0(V *, void *, void *);
void FUN_8003ec40(S *);

#define PB(o) ((char *)p + (o))

void FUN_8003e390(S *p) {
	V v20;
	SV aim;
	V v38;
	V v48;
	void *s3, *s2;
	V12 *q;
	unsigned short t, t2;
	int v, m, w;
	short a, b;

	t = p->uh21c;

	if (t) {
		p->uh21c = t - 1;
		p->uh24 = p->uh24 + p->uh222;
	}

	t = p->uh21e;

	if (t) {
		p->uh21e = t - 1;
		p->uh26 = p->uh26 + p->uh224;
	}

	t = p->uh220;

	if (t) {
		p->uh220 = t - 1;
		p->uh28 = p->uh28 + p->uh226;
	}

	p->i80 = 0;
	p->i7c = 0;
	p->i78 = 0;

	if (p->i23c == 0)
		p->i240 = p->i240 + 1;

	t = p->uh50;
	p->uh1a4 = t;

	if (t & 1) {
		p->uh50 = t & 0xfffe;
		p->ub1a0 = p->ub1a0 + 1;
	}

	if (p->i1ac & 0x10) {
		CPlatform_BouncyAI(p);
		return;
	}

	if (p->ub19c) {
		t2 = p->uh1fc;

		if (t2)
			p->uh1fc = t2 - 1;
		else
			CBaddy_ParseScript(p, p->i1b0);
	} else
		FUN_8003ec40(p);

	if (p->i1ac & 1) {
		s3 = PB(4);
		s2 = PB(0x204);
		__mi__FRC7CVectorT0(&v20, s3, s2);

		m = (p->uh1d4 >= CVector_Length(&v20));

		if (m) {
			*(V12 *)PB(4) = *(V12 *)PB(0x204);

			if (p->uh1d8 < 2) {
				q = (V12 *)PB(0x6c);
				q->z = 0;
				q->y = 0;
				p->i6c = 0;
			}

			p->i1ac = p->i1ac & ~1;
		} else {
			t = p->uh1d8;

			if (t == 0) {
				Utils_CalcAim(&aim, s3, s2);
				Utils_GetVecFromMagDir(PB(0x6c), p->uh1d4, &aim);
			} else if (t == 1) {
				Utils_CalcAim(&aim, s3, s2);
				__mi__FRC7CVectorT0(&v38, s2, s3);

				v = 8160 / CVector_Length(&v38);

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				*(SV *)&v38 = *(SV *)PB(0x10);
				Utils_TurnTowards(*(SV *)&v38, (SV *)PB(0x88), (SV *)PB(0x8e), aim, v);

				p->uh10 = 0;
				Utils_GetVecFromMagDir(PB(0x78), p->uh1d4, &aim);

				a = 0;

				if (p->i1ac & 0x20)
					a = ((unsigned short)aim.y - p->uh12) & 0xfff;

				if (a > 0x800)
					a = a - 0x1000;

				if (a > 0x400)
					a = 0x400;
				else if (a < -0x400)
					a = -0x400;

				w = -a;
				a = (w >> 1) & 0xfff;
				b = a - p->uh14;

				if (b < -0x800)
					b += 0x1000;

				if (b > 0x800)
					b -= 0x1000;

				if (b != 0)
					p->h92 = b >> 3;
				else
					p->h8c = 0;
			} else if (t == 2) {
				Utils_CalcAim(&aim, s3, s2);
				__mi__FRC7CVectorT0(&v48, s2, s3);

				v = 8160 / CVector_Length(&v48);

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				*(SV *)&v38 = *(SV *)PB(0x10);
				Utils_TurnTowards(*(SV *)&v38, (SV *)PB(0x88), (SV *)PB(0x8e), aim, v);

				p->uh10 = 0;
				Utils_GetVecFromMagDir(PB(0x78), p->uh1d4, &aim);
			}
		}
	} else if (p->uh1d8 == 1) {
		a = -p->uh14;

		if (a < -0x800)
			a += 0x1000;

		if (a > 0x800)
			a -= 0x1000;

		if (a == 0)
			p->h8c = 0;
		else
			p->h92 = a >> 3;
	}

	if (p->ub19e)
		__apl__7CVectorRC7CVector(PB(0x78), DAT_800a7f28);

	CPlatform_DoPhysics(p);

	if (p->i248 != -1 && TTime % p->i248 == 0)
		SFX_ModifyPos(p->i244, PB(4), 0);

	p->i23c = 0;
}
