/* Move__9CGlassBit (0x8002a7dc) */
/* C++ (BIT.CPP). A glass shard falls under gravity and bounces off the ground twice (re-scattering
   its corners), sparkling at random until then; after its hold time it fades out and dies. */
#include "game/bit.h"

int Redbook_Rnd(int);

void CGlassBit::Move() {
	int vx = mVel.vx;
	int vy = mVel.vy;
	int vz = mVel.vz;

	mPosA.vx += vx;
	mPosA.vy += vy;
	mPosA.vz += vz;
	mPosB.vx += vx;
	mPosB.vy += vy;
	mPosB.vz += vz;
	mPosC.vx += vx;
	mPosC.vy += vy;
	mPosC.vz += vz;
	vy += 10000;

	if (mGroundY < mPosA.vy) {
		if (mAge < 2) {
			mPosA.vy = mGroundY - (Redbook_Rnd(50) << 12);
			mPosB.vy = mGroundY - (Redbook_Rnd(50) << 12);
			vy = -vy >> 2;
			mPosC.vy = mGroundY - (Redbook_Rnd(50) << 12);
			mAge++;
		} else {
			vy = 0;
		}

		vx >>= 1;
		vz >>= 1;
	}

	mVel.vx = vx;
	mVel.vy = vy;
	mVel.vz = vz;

	if (mAge < 2 && Redbook_Rnd(5) == 0) {
		mR = 48;
		mG = 48;
		mB = 64;
	} else {
		mR = mDefaultR;
		mG = mDefaultG;
		mB = mDefaultB;
	}

	if (mLifetime == 0 || --mLifetime == 0) {
		mDefaultR = mDefaultR > mFadeRate ? mDefaultR - mFadeRate : 0;
		mDefaultG = mDefaultG > mFadeRate ? mDefaultG - mFadeRate : 0;
		mDefaultB = mDefaultB > mFadeRate ? mDefaultB - mFadeRate : 0;
	}

	if ((mDefaultR | mDefaultG | mDefaultB) == 0)
		Die();
}
