/* DoPhysics__8CPowerUp (0x8001ff84) */
#include "game/powerup.h"

struct TE {
	short x, y;
};

extern TE rcossin_tbl[2];
extern int TTime;

int Utils_GetGroundHeight(const CVector &, int, int);

void CPowerUp::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	if (mDropping) {
		if (!mDropForever && mGroundY < 0 && mGroundY >= -4) {
			if (TTime & 1) {
				int ground = Utils_GetGroundHeight(mOrgPos, 0, 8000);

				if (ground == -1) {
					mGroundY--;

					if (mGroundY < -4)
						mDropForever = 1;
				} else {
					mGroundY = ground;
				}
			}
		} else {
			mOrgPos += mVel;
			mVel.vy += mAcc.vy;
			mVel.vy -= mVel.vy >> mFric.vy;

			if (!mDropForever) {
				int bottom = mGroundY - (mHoverHeight << 12);

				if (bottom < mOrgPos.vy && mVel.vy > 0) {
					mOrgPos.vy = bottom;
					mVel.Set(0, 0, 0);

					mDropping = 0;
				}
			}

			mPos = mOrgPos;
		}
	} else {
		mPos = mOrgPos;
		int idx = mT & 4095;
		TE *b;

		b = rcossin_tbl;
		mPos.vy += ((mWobbleAmplitude * b[idx].x) / 4096) << 12;
		mT += mWobbleSpeed;
	}

	mShadowPos = mOrgPos;
	mShadowPos.vy = mGroundY;
	mShadowDist = (mGroundY - mPos.vy) >> 12;

	if (mShadowDist)
		mShadowScale = 140 - (mShadowDist * 60) / 128;
	else
		mShadowScale = 0;
}
