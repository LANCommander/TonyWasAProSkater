/* QFromV__FRC7CVector (0x80077408) */
/* C++ (QUAT.CPP): the quaternion for a rotation vector (axis * angle): identity for a zero
   vector, else sin/cos of half its length about its unit axis. Returns the CQuat by value. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/vector.h"

typedef struct {
	short s, c;
} ST;

extern ST rcossin_tbl[];

int M3dMaths_SquareRoot0(int a);

CQuat QFromV(const CVector &v) {
	int s;
	CVector t;
	int a, b, x, y, z;

	s = M3dMaths_SquareRoot0(v.vx * v.vx + v.vy * v.vy + v.vz * v.vz);

	if (!s)
		return CQuat(0, 0, 0, 4096);

	VectorNormal((VECTOR *)&v, (VECTOR *)&t);

	{
		ST *bb;
		int i = (s >> 1) & 4095;

		bb = rcossin_tbl;
		a = bb[i].s;
		b = bb[i].c;
	}

	x = (a * t.vx) >> 12;
	y = (a * t.vy) >> 12;
	z = (a * t.vz) >> 12;
	return CQuat(x, y, z, b);
}
