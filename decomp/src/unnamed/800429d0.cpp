/* FlipToVelocity__6CBruce (0x800429d0) size=224 */
/* C++ (PHYSICS.CPP-style CBruce method): flips the skater about the up axis by negating the
   front and right columns of the rotation matrix. The function finder split it in three
   (0x800429d0, 0x80042a54, 0x80042aa0). */
#include "game/bruce.h"

extern "C" int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);

void CBruce::FlipToVelocity() {
	if (mAnim == 34)
		return;

	if (Mips_CVecDotProd__FP7CVectorT0(&mFront, &mVel) <= 20480)
		return;

	mRotMatrix.m[0][2] = -mFront.vx;
	mFront.vx = mRotMatrix.m[0][2];
	mRotMatrix.m[1][2] = -mFront.vy;
	mFront.vy = mRotMatrix.m[1][2];
	mRotMatrix.m[2][2] = -mFront.vz;
	mFront.vz = mRotMatrix.m[2][2];
	mRotMatrix.m[0][0] = -mRight.vx;
	mRight.vx = mRotMatrix.m[0][0];
	mRotMatrix.m[1][0] = -mRight.vy;
	mRight.vy = mRotMatrix.m[1][0];
	mRotMatrix.m[2][0] = -mRight.vz;
	mRight.vz = mRotMatrix.m[2][0];
	mExtraFlags ^= 2;
}
