/* QUAT.CPP: CQuat maths: products, quaternions from rotation vectors and single-axis angles,
   conversions to and from matrices, and slerp (declarations in game/quat.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/quat.h"
#include "game/vector.h"
#include "game/trig.h"
#include "game/psyq.h"
#include "game/utils.h"

int M3dMaths_SquareRoot0(int a);
extern int DAT_800c110c[];
#define SIN(a) (((short *)rcossin_tbl)[((a) & 4095) * 2])
extern char DAT_800a3484[];
void __dl__6CClassPv(void *p);

/* __ml__FRC5CQuatT0 (0x8007722c) */
/* Exact demo twin (the demo reaches the vtables through gp): copy b, multiply in
   place by a with the inline operator*=, return by value. Locals (tempw..tempx, inline q) from MAIN.SYM. */
CQuat operator*(const CQuat &a, const CQuat &b) {
	CQuat q = b;

	q *= a;
	return q;
}

/* QFromV__FRC7CVector (0x80077408) */
/* : the quaternion for a rotation vector (axis * angle): identity for a zero
   vector, else sin/cos of half its length about its unit axis. Returns the CQuat by value. */
CQuat QFromV(const CVector &v) {
	int s;
	CVector t;
	int a, b, x, y, z;

	s = M3dMaths_SquareRoot0(v.vx * v.vx + v.vy * v.vy + v.vz * v.vz);

	if (!s)
		return CQuat(0, 0, 0, 4096);

	VectorNormal((VECTOR *)&v, (VECTOR *)&t);

	{
		SSinCos *bb;
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

/* QFromXRot__Fs (0x80077528) */
/* Exact demo twin: quaternion for a rotation about one axis. Locals from MAIN.SYM. */
/* declared <= 8 bytes: one la, as retail */
CQuat QFromXRot(short angle) {
	int cos2 = (rcossin_tbl + ((angle >> 1) & 4095))->c;
	int sin2 = (rcossin_tbl + ((angle >> 1) & 4095))->s;

	return CQuat(sin2, 0, 0, cos2);
}

/* QFromYRot__Fs (0x80077568) */
/* Exact demo twin: quaternion for a rotation about one axis. Locals from MAIN.SYM. */
/* declared <= 8 bytes: one la, as retail */
CQuat QFromYRot(short angle) {
	int cos2 = (rcossin_tbl + ((angle >> 1) & 4095))->c;
	int sin2 = (rcossin_tbl + ((angle >> 1) & 4095))->s;

	return CQuat(0, sin2, 0, cos2);
}

/* QFromZRot__Fs (0x800775a8) */
/* Exact demo twin: quaternion for a rotation about one axis. Locals from MAIN.SYM. */
/* declared <= 8 bytes: one la, as retail */
CQuat QFromZRot(short angle) {
	int cos2 = (rcossin_tbl + ((angle >> 1) & 4095))->c;
	int sin2 = (rcossin_tbl + ((angle >> 1) & 4095))->s;

	return CQuat(0, 0, sin2, cos2);
}

/* QToM__FRC5CQuatR6MATRIX (0x800775e8) */
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

/* MToQ__FRC6MATRIXR5CQuat (0x800776f8) */
void MToQ(const MATRIX &m, CQuat &q) {
	int tr, s, i, j, k;

	tr = m.m[0][0] + m.m[1][1] + m.m[2][2];

	if (tr > 0) {
		s = ::SquareRoot0((tr + 4096) << 12);
		q.w = s >> 1;

		if (s)
			s = 0x800000 / s;

		q.x = ((m.m[1][2] - m.m[2][1]) * s) >> 12;
		q.y = ((m.m[2][0] - m.m[0][2]) * s) >> 12;
		q.z = ((m.m[0][1] - m.m[1][0]) * s) >> 12;
	} else {
		i = m.m[0][0] < m.m[1][1];

		if (m.m[i][i] < m.m[2][2])
			i = 2;

		j = DAT_800c110c[i];
		k = DAT_800c110c[j];
		s = ::SquareRoot0(((m.m[i][i] - (m.m[j][j] + m.m[k][k])) + 4096) << 12);

		switch (i) {
			case 0:
				q.x = s >> 1;
				break;
			case 1:
				q.y = s >> 1;
				break;
			case 2:
				q.z = s >> 1;
				break;
		}

		if (s)
			s = 0x800000 / s;

		q.w = ((m.m[j][k] - m.m[k][j]) * s) >> 12;

		switch (j) {
			case 0:
				q.x = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
			case 1:
				q.y = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
			case 2:
				q.z = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
		}

		switch (k) {
			case 0:
				q.x = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
			case 1:
				q.y = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
			case 2:
				q.z = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
		}
	}
}

/* Quat_Slerp__FG5CQuatT0iR5CQuat (0x80077ac4) */
/* Fuzzy demo twin (tools/fuzzy_twins.py); locals from its MAIN.SYM. p and q are by-value
   CQuats (non-POD, so passed by hidden reference); the trailing vptr stores are their destructors
   (CClass is modelled without virtuals so g++ doesn't emit the out-of-line destructors here). */
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

/* FUN_80078088 (0x80078088) */
void FUN_80078088(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3484;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_800780bc (0x800780bc) */
void FUN_800780bc(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3484;

	if (f & 1)
		__dl__6CClassPv(p);
}
