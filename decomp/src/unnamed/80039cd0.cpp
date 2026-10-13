/* RotateHeadingAndVelocity__6CBruceiii (0x80039cd0) */
/* C++: CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE match_sym.mjs ... "-O2 -fno-exceptions" */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/m3dasm.h"
#include "game/vector.h"

typedef CVector V;

extern CVector operator*(const CVector &, const int &);
extern CVector operator/(const CVector &, const int &);

extern "C" char ZeroVector[];

void CBruce::RotateHeadingAndVelocity(int a, int c, int b) {
	MATRIX m;
	SVECTOR ang;
	MATRIX m3;
	MATRIX m2;
	V tmp;
	V r;
	int l0, l1;
	V *s1;

	if (a) {
		if (mInAir == 0) {
			if (!(m814 != *(V *)ZeroVector)) {
				ang.vz = 0;
				ang.vx = 0;
				ang.vy = a & 4095;
				RotMatrixYXZ(&ang, &m);
				MulMatrix(&mRotMatrix, &m);
				FRUVectorsFromRotMatrix();

				if (c) {
					s1 = &mVel;
					l0 = M3dMaths_SquareRoot0(Mips_CVecDotProd(s1, s1)) << 6;
					TransposeMatrix(&mOldRotMatrix, &m2);

					ang.vz = 0;
					ang.vx = 0;
					ang.vy = (a - b) & 4095;
					RotMatrixYXZ(&ang, &m);
					MulMatrix(&m2, &m);

					m3 = mOldRotMatrix;
					MulMatrix(&m3, &m2);

					tmp.vx = m3.m[0][0];
					tmp.vy = m3.m[0][1];
					tmp.vz = m3.m[0][2];
					r.vx = Mips_CVecDotProd(&tmp, s1);
					tmp.vx = m3.m[1][0];
					tmp.vy = m3.m[1][1];
					tmp.vz = m3.m[1][2];
					r.vy = Mips_CVecDotProd(&tmp, s1);
					tmp.vx = m3.m[2][0];
					tmp.vy = m3.m[2][1];
					tmp.vz = m3.m[2][2];
					r.vz = Mips_CVecDotProd(&tmp, s1);
					*s1 = r;
					l1 = M3dMaths_SquareRoot0(Mips_CVecDotProd(s1, s1)) << 6;
					l0 >>= 8;
					l1 >>= 8;
					*s1 = (*s1 * l0) / l1;
				}
			}
		}
	}
}
