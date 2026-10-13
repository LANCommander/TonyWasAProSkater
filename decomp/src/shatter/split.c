/* Split__FRC7CVectorN20iiiiiiUii (0x80020f1c) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short x, y, z, pad;
} CS;

extern int LowMemory;
extern int UseCacheForBits;

int pNormal;
unsigned short VRAMClut, VRAMTPage;
unsigned char AverageR, AverageG, AverageB;

extern void *__nw__4CBitUi(int);
extern void *CShatterBit_ctor(void *, CS *, CS *, CS *, V *, int);
extern int CVector_Length(V *);
extern int Redbook_Rnd(int);
extern void CChunkBit_SetRGB(void *, int, int, int);
extern void CChunkBit_SetUVs(void *, int, int, int, int, int, int, int, int);
extern void __als__7CVectorRCi(V *, int *);
extern void __apl__8CSVectorRC8CSVector(CS *, CS *);
extern void __mi__FRC7CVectorT0(V *, V *, V *);
extern void __ml__FRC7CVectorRCi(V *, V *, int *);
extern void __ml__FRCiRC8CSVector(CS *, int *, CS *);
extern void __pl__FRC7CVectorT0(V *, V *, V *);
extern void __rs__FRC7CVectorRCi(V *, V *, int *);
extern void __rs__FRC8CSVectorRCi(CS *, CS *, int *);

void Split(V *a0, V *a1, V *a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10) {
	V t30;
	int p30;
	V t40;
	int p40;
	V t50;
	int p50;
	V t60;
	int p60;
	V s70;
	int p70;
	V s80;
	int p80;
	CS c90, c98, cA0, cA8, cB0;
	int iB8, iBC, iC0, iC4, iC8, iCC, iD0, iD4, iD8, iDC, iE0, iE4, iE8, iEC;
	int s0, s1, l2, x7, x8;
	void *o;

	if (LowMemory == 0) {
		if (a10 != 0) {
			__mi__FRC7CVectorT0(&t40, a1, a2);

			t30 = t40;
			s1 = CVector_Length(&t30);
			s0 = 0;
			__mi__FRC7CVectorT0(&t40, a0, a2);

			t30 = t40;
			l2 = CVector_Length(&t30);

			if (s1 < l2) {
				s1 = l2;
				s0 = 1;
			}

			__mi__FRC7CVectorT0(&t40, a0, a1);

			t30 = t40;

			if (s1 < CVector_Length(&t30))
				s0 = 2;

			iB8 = Redbook_Rnd(9);

			switch (s0) {
				case 0: {
					int n;
					V *Pm;
					V *P50;
					V *P60;

					Pm = &s70;
					P50 = &t50;
					P60 = &t60;
					__mi__FRC7CVectorT0(Pm, a2, a1);
					__ml__FRC7CVectorRCi(P60, Pm, &iB8);

					iBC = 3;
					__rs__FRC7CVectorRCi(P50, P60, &iBC);
					__pl__FRC7CVectorT0(&t40, a1, P50);

					t30 = t40;
					n = a10 - 1;
					x7 = a5 + (((a7 - a5) * iB8) >> 3);
					x8 = a6 + (((a8 - a6) * iB8) >> 3);
					Split(a0, a1, &t30, a3, a4, a5, a6, x7, x8, a9, n);
					Split(a0, a2, &t30, a3, a4, a7, a8, x7, x8, a9, n);
					return;
				}
				case 1: {
					int n;
					V *Pm;
					V *P50;
					V *P60;

					Pm = &s80;
					P50 = &t50;
					P60 = &t60;
					__mi__FRC7CVectorT0(Pm, a2, a0);
					__ml__FRC7CVectorRCi(P60, Pm, &iB8);

					iC0 = 3;
					__rs__FRC7CVectorRCi(P50, P60, &iC0);
					__pl__FRC7CVectorT0(&t40, a0, P50);

					t30 = t40;
					n = a10 - 1;
					x7 = a3 + (((a7 - a3) * iB8) >> 3);
					x8 = a4 + (((a8 - a4) * iB8) >> 3);
					Split(a1, a0, &t30, a5, a6, a3, a4, x7, x8, a9, n);
					Split(a1, a2, &t30, a5, a6, a7, a8, x7, x8, a9, n);
					return;
				}
				case 2: {
					int n;
					V *Pm;
					V *P50;
					V *P60;

					Pm = &s70;
					P50 = &t50;
					P60 = &t60;
					__mi__FRC7CVectorT0(Pm, a1, a0);
					__ml__FRC7CVectorRCi(P60, Pm, &iB8);

					iC4 = 3;
					__rs__FRC7CVectorRCi(P50, P60, &iC4);
					__pl__FRC7CVectorT0(&t40, a0, P50);

					t30 = t40;
					n = a10 - 1;
					x7 = a3 + (((a5 - a3) * iB8) >> 3);
					x8 = a4 + (((a6 - a4) * iB8) >> 3);
					Split(a2, a0, &t30, a7, a8, a3, a4, x7, x8, a9, n);
					Split(a2, a1, &t30, a7, a8, a5, a6, x7, x8, a9, n);
					return;
				}
			}
		} else {
			CS *pA0;
			CS *pA8;
			CS *pB0;

			iC8 = 12;
			__rs__FRC7CVectorRCi(&t30, a0, &iC8);

			iCC = 12;
			__rs__FRC7CVectorRCi(&t40, a1, &iCC);

			iD0 = 12;
			__rs__FRC7CVectorRCi(&t50, a2, &iD0);

			t60.x = (t30.x + t40.x + t50.x) / 3;
			t60.y = (t30.y + t40.y + t50.y) / 3;
			t60.z = (t30.z + t40.z + t50.z) / 3;
			((CS *)&s70)->x = (unsigned short)t30.x - (unsigned short)t60.x;
			((CS *)&s70)->y = (unsigned short)t30.y - (unsigned short)t60.y;
			((CS *)&s70)->z = (unsigned short)t30.z - (unsigned short)t60.z;
			c90.x = (unsigned short)t40.x - (unsigned short)t60.x;
			c90.y = (unsigned short)t40.y - (unsigned short)t60.y;
			c90.z = (unsigned short)t40.z - (unsigned short)t60.z;
			c98.x = (unsigned short)t50.x - (unsigned short)t60.x;
			c98.y = (unsigned short)t50.y - (unsigned short)t60.y;
			c98.z = (unsigned short)t50.z - (unsigned short)t60.z;
			pA0 = &cA0;
			pA8 = &cA8;
			iD4 = Redbook_Rnd(100);
			__ml__FRCiRC8CSVector(pA8, &iD4, (CS *)pNormal);

			iD8 = 12;
			__rs__FRC8CSVectorRCi(pA0, pA8, &iD8);
			__apl__8CSVectorRC8CSVector((CS *)&s70, pA0);

			pB0 = &cB0;
			iDC = Redbook_Rnd(100);
			__ml__FRCiRC8CSVector(pB0, &iDC, (CS *)pNormal);

			iE0 = 12;
			__rs__FRC8CSVectorRCi(pA0, pB0, &iE0);
			__apl__8CSVectorRC8CSVector(&c90, pA0);

			iE4 = Redbook_Rnd(100);
			__ml__FRCiRC8CSVector(pA8, &iE4, (CS *)pNormal);

			iE8 = 12;
			__rs__FRC8CSVectorRCi(pA0, pA8, &iE8);
			__apl__8CSVectorRC8CSVector(&c98, pA0);

			iEC = 12;
			__als__7CVectorRCi(&t60, &iEC);

			UseCacheForBits = 0;
			o = __nw__4CBitUi(204);

			if (o)
				o = CShatterBit_ctor(o, (CS *)&s70, &c90, &c98, &t60, 0);

			UseCacheForBits = 1;
			CChunkBit_SetRGB(o, AverageR, AverageG, AverageB);
			CChunkBit_SetUVs(o, VRAMClut, VRAMTPage, a3 & 255, a4 & 255, a5 & 255, a6 & 255,
			                 a7 & 255, a8 & 255);

			*(int *)((char *)o + 0xbc) = a9;
		}
	}
}
