/* QToM__FRC5CQuatR6MATRIX (0x800775e8) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/vector.h"

void QToM(const CQuat &q, MATRIX &m) {
	int wx, wy, wz, xx, xy, xz, yy, yz, zz;

	wx = (q.w * q.x) >> 11;
	wy = (q.w * q.y) >> 11;
	wz = (q.w * q.z) >> 11;
	xx = (q.x * q.x) >> 11;
	xy = (q.x * q.y) >> 11;
	xz = (q.x * q.z) >> 11;
	yy = (q.y * q.y) >> 11;
	yz = (q.y * q.z) >> 11;
	zz = (q.z * q.z) >> 11;
	m.m[0][0] = 4096 - (yy + zz);
	m.m[0][1] = xy + wz;
	m.m[0][2] = xz - wy;
	m.m[1][0] = xy - wz;
	m.m[1][1] = 4096 - (xx + zz);
	m.m[1][2] = yz + wx;
	m.m[2][0] = xz + wy;
	m.m[2][1] = yz - wx;
	m.m[2][2] = 4096 - (xx + yy);
	m.t[0] = 0;
	m.t[1] = 0;
	m.t[2] = 0;
}
