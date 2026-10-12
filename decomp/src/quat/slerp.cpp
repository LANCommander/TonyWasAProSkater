/* Quat_Slerp__FG5CQuatT0iR5CQuat (0x80077ac4) */
/* C++ (QUAT.CPP). Fuzzy demo twin (tools/fuzzy_twins.py); locals from its MAIN.SYM. p and q are by-value
   CQuats (non-POD, so passed by hidden reference); the trailing vptr stores are their destructors
   (CClass is modelled without virtuals so g++ doesn't emit the out-of-line destructors here). */
#include "game/vector.h"
#include "game/trig.h"

extern SSinCos rcossin_tbl[1];

#define SIN(a) (((short *)rcossin_tbl)[((a) & 4095) * 2])

int Utils_ArcCos(int);

void Quat_Slerp(CQuat p, CQuat q, int t, CQuat &qt) {
	int omega, cosom, sinom, sclp, sclq;

	p.Normalise();
	q.Normalise();

	cosom = (p.x * q.x + p.y * q.y + p.z * q.z + p.w * q.w) >> 12;

	if (cosom < 0) {
		cosom = -cosom;
		p.x = -p.x;
		p.y = -p.y;
		p.z = -p.z;
		p.w = -p.w;
	}

	if (cosom > 4096)
		cosom = 4096;

	if (4096 + cosom > 128) {
		if (4096 - cosom > 128) {
			omega = Utils_ArcCos(cosom);
			sinom = SIN(omega);
			sclp = (SIN(((4096 - t) * omega) >> 12) << 12) / sinom;
			sclq = (SIN((t * omega) >> 12) << 12) / sinom;
		} else {
			sclp = 4096 - t;
			sclq = t;
		}

		qt.x = (sclp * p.x + sclq * q.x) >> 12;
		qt.y = (sclp * p.y + sclq * q.y) >> 12;
		qt.z = (sclp * p.z + sclq * q.z) >> 12;
		qt.w = (sclp * p.w + sclq * q.w) >> 12;
	} else {
		qt.x = -p.y;
		qt.y = -p.x;
		qt.z = -p.w;
		qt.w = p.z;
		sclp = SIN((unsigned)((4096 - t) * 3217) >> 11);
		sclq = SIN((unsigned)(t * 3217) >> 11);
		qt.x = (sclp * p.x + sclq * qt.x) >> 12;
		qt.y = (sclp * p.y + sclq * qt.y) >> 12;
		qt.z = (sclp * p.z + sclq * qt.z) >> 12;
		qt.w = (sclp * p.w + sclq * qt.w) >> 12;
	}

	qt.Normalise();
}
