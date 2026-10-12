/* Move__5CFrag (0x80029c8c) */
/* C++ (BIT.CPP). A fragment flies with acceleration and per-axis shift friction, steps its frame
   up to (and holding at) the last one, and dies when its lifetime runs out. */
#include "game/bit.h"

void CFrag::Move() {
	mPos.vx += mVel.vx;
	mPos.vy += mVel.vy;
	mPos.vz += mVel.vz;
	mVel.vx += mAcc.vx;
	mVel.vy += mAcc.vy;
	mVel.vz += mAcc.vz;
	mVel.vx -= mVel.vx >> mFric.vx;
	mVel.vy -= mVel.vy >> mFric.vy;
	mVel.vz -= mVel.vz >> mFric.vz;
	mFrame++;

	if (mFrame >= mNumFrames)
		mFrame = mNumFrames - 1;

	mAge++;

	if (mAge >= mLifetime)
		Die();
}
