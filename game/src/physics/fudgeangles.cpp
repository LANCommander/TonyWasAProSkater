/* FudgeAngles__FP6CBruce (0x80044b90) */
#include "game/bruce.h"

void Utils_RotateY(CVector &, const CVector &, int);

void FudgeAngles(CBruce *t) {
	CVector a, r;
	int ang;

	t->mAngles.vz = 0;
	t->mAngles.vx = 0;
	a.vx = -4096;
	a.vy = 0;
	a.vz = 0;
	ang = ((ushort)t->mAngles.vy + 0x800) & 0xfff;
	t->mAngles.vy = ang;
	Utils_RotateY(r, a, ang);

	a.vx = 0;
	a.vy = 0;
	a.vz = -4096;
	t->mRotMatrix.m[0][0] = r.vx;
	t->mRotMatrix.m[1][0] = r.vy;
	t->mRotMatrix.m[2][0] = r.vz;
	Utils_RotateY(r, a, t->mAngles.vy);

	t->mRotMatrix.m[1][1] = -4096;
	t->mUp.vy = -4096;
	t->mRotMatrix.m[0][1] = 0;
	t->mUp.vx = 0;
	t->mRotMatrix.m[2][1] = 0;
	t->mUp.vz = 0;
	t->mRotMatrix.m[0][2] = r.vx;
	t->mFront.vx = (short)r.vx;
	t->mRotMatrix.m[1][2] = r.vy;
	t->mFront.vy = (short)r.vy;
	t->mRotMatrix.m[2][2] = r.vz;
	t->mFront.vz = (short)r.vz;
}
