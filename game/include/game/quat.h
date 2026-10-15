/* QUAT.H: CQuat maths: products, quaternions from rotation vectors and single-axis angles,
   conversions to and from matrices, and slerp (QUAT.CPP). */
#ifndef GAME_QUAT_H
#define GAME_QUAT_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"
#include "game/vector.h"
#include "game/class.h"

/* CQuat: vptr at +0 (CClass), x/y/z/w 20.12 fixed point. Retail's vtable 0x800a3474 has one slot,
   ~CQuat (inline, QUAT.H:132), which TUs emit locally (0x80078088, 0x8007d944). */
class CQuat : public CClass {
  public:
	int x; /* 0x04 */
	int y; /* 0x08 */
	int z; /* 0x0c */
	int w; /* 0x10 */

	CQuat() { /* identity (CAMERA's static initialiser 0x8007d6e4) */
		x = 0;
		y = 0;
		z = 0;
		w = 4096;
	}
	CQuat(int a, int b, int c, int d) {
		x = a;
		y = b;
		z = c;
		w = d;
	}
	/* slot 0: the implicit (synthesized) ~CQuat: retail's copies store only the inlined ~CClass's
	   vptr, never CQuat's own (an explicit `{}` body would add that store) */

	CQuat &operator*=(const CQuat &q) {
		int tempw = (w * q.w - x * q.x - y * q.y - z * q.z) >> 12;
		int tempx = (w * q.x + x * q.w + y * q.z - z * q.y) >> 12;
		int tempy = (w * q.y + y * q.w + z * q.x - x * q.z) >> 12;
		int tempz = (w * q.z + z * q.w + x * q.y - y * q.x) >> 12;

		x = tempx;
		y = tempy;
		z = tempz;
		w = tempw;
		return *this;
	}
	void Normalise() {
		int mag = SquareRoot0(x * x + y * y + z * z + w * w);

		if (!mag)
			mag = 1;

		x = (x << 12) / mag;
		y = (y << 12) / mag;
		z = (z << 12) / mag;
		w = (w << 12) / mag;
	}
};

/* functions */
CQuat operator*(const CQuat &a, const CQuat &b);
CQuat QFromV(const CVector &v);
CQuat QFromXRot(short angle);
CQuat QFromYRot(short angle);
CQuat QFromZRot(short angle);
void QToM(const CQuat &q, MATRIX &m);
void MToQ(const MATRIX &m, CQuat &q);
void Quat_Slerp(CQuat p, CQuat q, int t, CQuat &qt);

#endif
