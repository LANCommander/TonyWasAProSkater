/* DoPhysics__9CPlatform (0x8003e9d8) */
#include "game/platform.h"

void CPlatform::DoPhysics() {
	mNormal.vz = 0;
	mNormal.vy = 0;
	mNormal.vx = 0;

	if (mBaddyAttributes[2] == 0) {
		int vx, vy, vz;
		short sx, sy, sz;

		vx = mVel.vx + mAcc.vx;
		vy = mVel.vy + mAcc.vy;
		vz = mVel.vz + mAcc.vz;

		if (vx >= -2048 && vx <= 2048)
			vx = 0;

		if (vy >= -2048 && vy <= 2048)
			vy = 0;

		if (vz >= -2048 && vz <= 2048)
			vz = 0;

		mVel.vx = vx;
		mVel.vy = vy;
		mVel.vz = vz;
		mPos.vx += vx;
		mPos.vy += vy;
		mPos.vz += vz;
		sx = mAngVel.vx;
		sy = mAngVel.vy;
		sz = mAngVel.vz;
		mAngles.vx = (mAngles.vx + sx) & 4095;
		mAngles.vy = (mAngles.vy + sy) & 4095;
		mAngles.vz = (mAngles.vz + sz) & 4095;
		sx = sx + mAngAcc.vx;
		sy = sy + mAngAcc.vy;
		sz = sz + mAngAcc.vz;

		if (sx >= -1 && sx <= 1)
			sx = 0;

		if (sy >= -1 && sy <= 1)
			sy = 0;

		if (sz >= -1 && sz <= 1)
			sz = 0;

		mAngVel.vx = sx;
		mAngVel.vy = sy;
		mAngVel.vz = sz;
	} else if (mBaddyAttributes[2] == 1 || mBaddyAttributes[2] == 2) {
		mVel += mAcc;
		mVel %= mFric;
		mVel.KillSmall();

		mPos += mVel;
		mAngles += mAngVel;
		mAngles.Mask();

		mAngVel += mAngAcc;
		mAngVel %= mAngFric;
		mAngVel.KillSmall();
	}
}
