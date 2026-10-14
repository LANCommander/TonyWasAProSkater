/* OrientToNormal__6CBruce (0x8003b0c0) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

void M3dUtils_NormaliseVector(CVector *);

void CBruce::OrientToNormal() {
	VECTOR z1;
	CVector normal;
	CVector x1;

	if (mSameNormal < 25) {
		if (mSameNormal == 24) {
			int _x = mNormal.vx;
			int _y = mNormal.vy;
			int _z = mNormal.vz;

			normal.vx = _x;
			normal.vy = _y;
			normal.vz = _z;
		} else {
			normal.vx = mOldNormal.vx + ((mNormal.vx - mOldNormal.vx) >> 2);
			normal.vy = mOldNormal.vy + ((mNormal.vy - mOldNormal.vy) >> 2);
			normal.vz = mOldNormal.vz + ((mNormal.vz - mOldNormal.vz) >> 2);
		}

		M3dUtils_NormaliseVector(&normal);
		mFront.Set(mRotMatrix.m[0][2], mRotMatrix.m[1][2], mRotMatrix.m[2][2]);
		geom_cross(&mFront, &normal, &x1, 12);
		M3dUtils_NormaliseVector(&x1);
		geom_cross(&normal, &x1, &z1, 12);

		mRotMatrix.m[0][0] = x1.vx;
		mRight.vx = mRotMatrix.m[0][0];
		mRotMatrix.m[1][0] = x1.vy;
		mRight.vy = mRotMatrix.m[1][0];
		mRotMatrix.m[2][0] = x1.vz;
		mRight.vz = mRotMatrix.m[2][0];
		mRotMatrix.m[0][1] = normal.vx;
		mUp.vx = mRotMatrix.m[0][1];
		mRotMatrix.m[1][1] = normal.vy;
		mUp.vy = mRotMatrix.m[1][1];
		mRotMatrix.m[2][1] = normal.vz;
		mUp.vz = mRotMatrix.m[2][1];
		mRotMatrix.m[0][2] = z1.vx;
		mFront.vx = mRotMatrix.m[0][2];
		mRotMatrix.m[1][2] = z1.vy;
		mFront.vy = mRotMatrix.m[1][2];
		mRotMatrix.m[2][2] = z1.vz;
		mFront.vz = mRotMatrix.m[2][2];

		{
			int _x = normal.vx;
			int _y = normal.vy;
			int _z = normal.vz;

			mOldNormal.vx = _x;
			mOldNormal.vy = _y;
			mOldNormal.vz = _z;
		}
	}
}
