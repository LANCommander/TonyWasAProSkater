/* Move__6CSpark (0x80029810) */
/* C++ (BIT.CPP). A spark falls under gravity with 1/8 friction until it reaches its floor (if it has
   one), fading its colour each frame, and dies when its lifetime runs out. */
#include "game/bit.h"

void CSpark::Move() {
	int x = mPos.vx + mVel.vx;
	int y = mPos.vy + mVel.vy;
	int z = mPos.vz + mVel.vz;

	if (mFloorY && mFloorY < y) {
		mPos.vy = mFloorY;
	} else {
		mPos.vx = x;
		mPos.vy = y;
		mPos.vz = z;

		x = mVel.vx + mAcc.vx;
		y = mVel.vy + mAcc.vy;
		z = mVel.vz + mAcc.vz;
		mVel.vx = x - (x >> 3);
		mVel.vy = y - (y >> 3);
		mVel.vz = z - (z >> 3);
	}

	mAge++;

	if (mAge >= mLifetime)
		Die();

	r0 -= mFadeR;
	g0 -= mFadeG;
	b0 -= mFadeB;
}
