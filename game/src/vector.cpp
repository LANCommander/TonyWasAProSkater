/* VECTOR.CPP: the out-of-line members and operators of CVector, CSVector and CFriction
   (declarations in game/vector.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/vector.h"
#include "game/camera.h"
#include "game/campt.h"
#include "game/m3dasm.h"

/* Length__C7CVector (0x8004d900) */
/* GTE SQR of the >>12 components, then SquareRoot0 (0x8008b5dc). */
int CVector::Length() const {
	VECTOR t;

	t.vx = vx >> 12;
	t.vy = vy >> 12;
	t.vz = vz >> 12;
	geom_square(&t, &t);
	return SquareRoot0(t.vx + t.vy + t.vz);
}

/* Normalize__7CVector (0x8004d980) */
/* Divides by Length(). Retail never reloads z: it stores (y/len)/len there (an
   original bug). */
void CVector::Normalize() {
	int l = Length();

	vx = vx / l;
	vy = vy / l;
	vz = vy / l;
}

/* KillSmall__7CVector (0x8004d9e0) */
/* Zeroes each component in [-0x800, 0x800]. */
void CVector::KillSmall() {
	if ((unsigned)(vx + 0x800) < 0x1001)
		vx = 0;

	if ((unsigned)(vy + 0x800) < 0x1001)
		vy = 0;

	if ((unsigned)(vz + 0x800) < 0x1001)
		vz = 0;
}

/* __apl__7CVectorRC7CVector (0x8004da3c) */
/* Component-wise add. */
CVector &CVector::operator+=(const CVector &v) {
	vx += v.vx;
	vy += v.vy;
	vz += v.vz;
	return *this;
}

/* __ami__7CVectorRC7CVector (0x8004da78) */
/* Component-wise subtract. */
CVector &CVector::operator-=(const CVector &v) {
	vx -= v.vx;
	vy -= v.vy;
	vz -= v.vz;
	return *this;
}

/* __aml__7CVectorRCi (0x8004dab4) */
/* Multiplies each component by n. */
CVector &CVector::operator*=(const int &n) {
	vx *= n;
	vy *= n;
	vz *= n;
	return *this;
}

/* __adv__7CVectorRCi (0x8004db04) */
/* Divides each component by n. */
CVector &CVector::operator/=(const int &n) {
	vx /= n;
	vy /= n;
	vz /= n;
	return *this;
}

/* __ars__7CVectorRCi (0x8004db54) */
/* Shifts each component right (arithmetic) by n. */
CVector &CVector::operator>>=(const int &n) {
	vx >>= n;
	vy >>= n;
	vz >>= n;
	return *this;
}

/* __als__7CVectorRCi (0x8004db90) */
/* Shifts each component left by n. */
CVector &CVector::operator<<=(const int &n) {
	vx <<= n;
	vy <<= n;
	vz <<= n;
	return *this;
}

/* __amd__7CVectorRC9CFriction (0x8004dbcc) */
/* Applies friction: each component loses itself >> its friction shift. */
CVector &CVector::operator%=(const CFriction &f) {
	vx -= vx >> f.vx;
	vy -= vy >> f.vy;
	vz -= vz >> f.vz;
	return *this;
}

/* __eq__7CVectorRC7CVector (0x8004dc18) */
/* Component-wise equality. */
int CVector::operator==(const CVector &v) {
	return vx == v.vx && vy == v.vy && vz == v.vz;
}

/* __ne__7CVectorRC7CVector (0x8004dc5c) */
/* Component-wise inequality. */
int CVector::operator!=(const CVector &v) {
	return vx != v.vx || vy != v.vy || vz != v.vz;
}

/* __pl__FRC7CVectorT0 (0x8004dca4) */
CVector operator+(const CVector &a, const CVector &b) {
	int x = a.vx + b.vx, y = a.vy + b.vy, z = a.vz + b.vz;

	return CVector(x, y, z);
}

/* __mi__FRC7CVectorT0 (0x8004dcfc) */
CVector operator-(const CVector &a, const CVector &b) {
	int x = a.vx - b.vx, y = a.vy - b.vy, z = a.vz - b.vz;

	return CVector(x, y, z);
}

/* __ml__FRC7CVectori (0x8004dd54) */
CVector operator*(const CVector &a, const int &n) {
	int x = a.vx * n, y = a.vy * n, z = a.vz * n;

	return CVector(x, y, z);
}

/* __ml__FiRC7CVector (0x8004ddbc) */
CVector operator*(const int &n, const CVector &a) {
	int x = a.vx * n, y = a.vy * n, z = a.vz * n;

	return CVector(x, y, z);
}

/* __dv__FRC7CVectori (0x8004de24) */
CVector operator/(const CVector &a, const int &n) {
	int x = a.vx / n, y = a.vy / n, z = a.vz / n;

	return CVector(x, y, z);
}

/* __rs__FRC7CVectori (0x8004de8c) */
CVector operator>>(const CVector &a, const int &n) {
	int x = a.vx >> n, y = a.vy >> n, z = a.vz >> n;

	return CVector(x, y, z);
}

/* __ls__FRC7CVectori (0x8004dedc) */
CVector operator<<(const CVector &a, const int &n) {
	int x = a.vx << n, y = a.vy << n, z = a.vz << n;

	return CVector(x, y, z);
}

/* __mi__C7CVector (0x8004df2c) */
/* Unary minus: returns the negated vector. */
CVector CVector::operator-() const {
	CVector r;
	int x = -vx, y = -vy, z = -vz;

	r.vx = x;
	r.vy = y;
	r.vz = z;
	return r;
}

/* Mask__8CSVector (0x8004df58) */
/* Masks each component (an angle) to 12 bits. */
void CSVector::Mask() {
	vx &= 0xfff;
	vy &= 0xfff;
	vz &= 0xfff;
}

/* KillSmall__8CSVector (0x8004df80) */
/* Zeroes each component that is -1, 0 or 1. */
void CSVector::KillSmall() {
	if ((unsigned short)(vx + 1) < 3)
		vx = 0;

	if ((unsigned short)(vy + 1) < 3)
		vy = 0;

	if ((unsigned short)(vz + 1) < 3)
		vz = 0;
}

/* __apl__8CSVectorRC8CSVector (0x8004dfe8) */
/* Component-wise add (truncated to short). */
CSVector &CSVector::operator+=(const CSVector &v) {
	vx += v.vx;
	vy += v.vy;
	vz += v.vz;
	return *this;
}

/* __adv__8CSVectorRCi (0x8004e024) */
/* Divides each component by n (truncated back to short). */
CSVector &CSVector::operator/=(const int &n) {
	vx /= n;
	vy /= n;
	vz /= n;
	return *this;
}

/* __amd__8CSVectorRC9CFriction (0x8004e074) */
/* Applies friction: each component loses itself >> its friction shift. */
CSVector &CSVector::operator%=(const CFriction &f) {
	vx -= vx >> f.vx;
	vy -= vy >> f.vy;
	vz -= vz >> f.vz;
	return *this;
}

/* __mi__FRC8CSVectorT0 (0x8004e0c4) */
CSVector operator-(const CSVector &a, const CSVector &b) {
	short x = a.vx - b.vx, y = a.vy - b.vy, z = a.vz - b.vz;

	return CSVector(x, y, z);
}

/* __ml__FRCiRC8CSVector (0x8004e11c) */
CSVector operator*(const int &n, const CSVector &a) {
	short x = a.vx * n, y = a.vy * n, z = a.vz * n;

	return CSVector(x, y, z);
}

/* __rs__FRC8CSVectorRCi (0x8004e184) */
CSVector operator>>(const CSVector &a, const int &n) {
	short x = a.vx >> n, y = a.vy >> n, z = a.vz >> n;

	return CSVector(x, y, z);
}
