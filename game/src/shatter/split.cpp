/* Split__FRC7CVectorN20iiiiiiUii (0x80020f1c) */
#include "game/bit.h"
#include "game/redbook.h"
#include "game/vector.h"

extern int LowMemory;
extern int UseCacheForBits;

extern CSVector *pNormal;
extern unsigned short VRAMClut, VRAMTPage;
extern unsigned char AverageR, AverageG, AverageB;

void Split(const CVector &a0, const CVector &a1, const CVector &a2, int a3, int a4, int a5, int a6,
           int a7, int a8, uint a9, int a10) {
	CVector t30;
	int s0, s1, l2, x7, x8, r;

	if (LowMemory == 0) {
		if (a10 != 0) {
			t30 = a1 - a2;
			s1 = t30.Length();
			s0 = 0;

			t30 = a0 - a2;
			l2 = t30.Length();

			if (s1 < l2) {
				s1 = l2;
				s0 = 1;
			}

			t30 = a0 - a1;

			if (s1 < t30.Length())
				s0 = 2;

			r = Redbook_Rnd(9);

			switch (s0) {
				case 0: {
					int n;

					t30 = a1 + (((a2 - a1) * r) >> 3);
					n = a10 - 1;
					x7 = a5 + (((a7 - a5) * r) >> 3);
					x8 = a6 + (((a8 - a6) * r) >> 3);
					Split(a0, a1, t30, a3, a4, a5, a6, x7, x8, a9, n);
					Split(a0, a2, t30, a3, a4, a7, a8, x7, x8, a9, n);
					return;
				}
				case 1: {
					int n;

					t30 = a0 + (((a2 - a0) * r) >> 3);
					n = a10 - 1;
					x7 = a3 + (((a7 - a3) * r) >> 3);
					x8 = a4 + (((a8 - a4) * r) >> 3);
					Split(a1, a0, t30, a5, a6, a3, a4, x7, x8, a9, n);
					Split(a1, a2, t30, a5, a6, a7, a8, x7, x8, a9, n);
					return;
				}
				case 2: {
					int n;

					t30 = a0 + (((a1 - a0) * r) >> 3);
					n = a10 - 1;
					x7 = a3 + (((a5 - a3) * r) >> 3);
					x8 = a4 + (((a6 - a4) * r) >> 3);
					Split(a2, a0, t30, a7, a8, a3, a4, x7, x8, a9, n);
					Split(a2, a1, t30, a7, a8, a5, a6, x7, x8, a9, n);
					return;
				}
			}
		} else {
			CVector t40, t50, t60;
			CSVector p0, p1, p2;
			CShatterBit *o;

			t30 = a0 >> 12;
			t40 = a1 >> 12;
			t50 = a2 >> 12;

			t60.vx = (t30.vx + t40.vx + t50.vx) / 3;
			t60.vy = (t30.vy + t40.vy + t50.vy) / 3;
			t60.vz = (t30.vz + t40.vz + t50.vz) / 3;
			p0.vx = (unsigned short)t30.vx - (unsigned short)t60.vx;
			p0.vy = (unsigned short)t30.vy - (unsigned short)t60.vy;
			p0.vz = (unsigned short)t30.vz - (unsigned short)t60.vz;
			p1.vx = (unsigned short)t40.vx - (unsigned short)t60.vx;
			p1.vy = (unsigned short)t40.vy - (unsigned short)t60.vy;
			p1.vz = (unsigned short)t40.vz - (unsigned short)t60.vz;
			p2.vx = (unsigned short)t50.vx - (unsigned short)t60.vx;
			p2.vy = (unsigned short)t50.vy - (unsigned short)t60.vy;
			p2.vz = (unsigned short)t50.vz - (unsigned short)t60.vz;
			r = Redbook_Rnd(100);
			p0 += (r * *pNormal) >> 12;
			r = Redbook_Rnd(100);
			p1 += (r * *pNormal) >> 12;
			r = Redbook_Rnd(100);
			p2 += (r * *pNormal) >> 12;
			t60 <<= 12;

			UseCacheForBits = 0;
			o = new CShatterBit(p0, p1, p2, t60, 0);
			UseCacheForBits = 1;
			o->SetRGB(AverageR, AverageG, AverageB);
			o->SetUVs(VRAMClut, VRAMTPage, a3 & 255, a4 & 255, a5 & 255, a6 & 255, a7 & 255,
			          a8 & 255);
			o->mTexWin = a9;
		}
	}
}
