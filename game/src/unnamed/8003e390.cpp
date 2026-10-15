/* FUN_8003e390 (0x8003e390) */
/* C++ (PLATFORM.CPP): CPlatform::AI (vtable slot 2). */
#include "game/platform.h"

extern char DAT_800a7f28[];
extern int TTime;

extern "C" void FUN_8003ec40(CPlatform *);
void SFX_ModifyPos(unsigned int, const CVector &, int);
void Utils_CalcAim(CSVector &, const CVector &, const CVector &);
void Utils_GetVecFromMagDir(CVector &, int, const CSVector &);
void Utils_TurnTowards(CSVector, CSVector &, CSVector &, CSVector, int);

void CPlatform::AI() {
	CSVector aim;
	CSVector ang;
	CVector *s3, *s2;
	unsigned short t, t2;
	int v, m, w;
	short a, b;

	t = mScaleXTimer;

	if (t) {
		mScaleXTimer = t - 1;
		mScale.vx = (ushort)mScale.vx + mScaleXAdjust;
	}

	t = mScaleYTimer;

	if (t) {
		mScaleYTimer = t - 1;
		mScale.vy = (ushort)mScale.vy + mScaleYAdjust;
	}

	t = mScaleZTimer;

	if (t) {
		mScaleZTimer = t - 1;
		mScale.vz = (ushort)mScale.vz + mScaleZAdjust;
	}

	mAcc.vz = 0;
	mAcc.vy = 0;
	mAcc.vx = 0;

	if (mTrodUpon == 0)
		mNotTrodUponTimer = mNotTrodUponTimer + 1;

	t = mInputFlags;
	mInputFlagsCopy = t;

	if (t & 1) {
		mInputFlags = t & 0xfffe;
		mInputSignal = mInputSignal + 1;
	}

	if (mSpecificFlags & 0x10) {
		BouncyAI();
		return;
	}

	if (mDoScript) {
		t2 = mWaitTimer;

		if (t2)
			mWaitTimer = t2 - 1;
		else
			ParseScript(mpScript);
	} else
		FUN_8003ec40(this);

	if (mSpecificFlags & 1) {
		s3 = &mPos;
		s2 = &mTargetPos;
		CVector v20 = *s3 - *s2;

		m = (mBaddyAttributes[0] >= v20.Length());

		if (m) {
			mPos = mTargetPos;

			if (mBaddyAttributes[2] < 2) {
				mVel.vz = 0;
				mVel.vy = 0;
				mVel.vx = 0;
			}

			mSpecificFlags = mSpecificFlags & ~1;
		} else {
			t = mBaddyAttributes[2];

			if (t == 0) {
				Utils_CalcAim(aim, *s3, *s2);
				Utils_GetVecFromMagDir(mVel, mBaddyAttributes[0], aim);
			} else if (t == 1) {
				Utils_CalcAim(aim, *s3, *s2);
				CVector v38 = *s2 - *s3;

				v = 8160 / v38.Length();

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				ang = mAngles;
				Utils_TurnTowards(ang, mAngVel, mAngAcc, aim, v);

				mAngles.vx = 0;
				Utils_GetVecFromMagDir(mAcc, mBaddyAttributes[0], aim);

				a = 0;

				if (mSpecificFlags & 0x20)
					a = ((unsigned short)aim.vy - (ushort)mAngles.vy) & 0xfff;

				if (a > 0x800)
					a = a - 0x1000;

				if (a > 0x400)
					a = 0x400;
				else if (a < -0x400)
					a = -0x400;

				w = -a;
				a = (w >> 1) & 0xfff;
				b = a - (ushort)mAngles.vz;

				if (b < -0x800)
					b += 0x1000;

				if (b > 0x800)
					b -= 0x1000;

				if (b != 0)
					mAngAcc.vz = b >> 3;
				else
					mAngVel.vz = 0;
			} else if (t == 2) {
				Utils_CalcAim(aim, *s3, *s2);
				CVector v48 = *s2 - *s3;

				v = 8160 / v48.Length();

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				ang = mAngles;
				Utils_TurnTowards(ang, mAngVel, mAngAcc, aim, v);

				mAngles.vx = 0;
				Utils_GetVecFromMagDir(mAcc, mBaddyAttributes[0], aim);
			}
		}
	} else if (mBaddyAttributes[2] == 1) {
		a = -(ushort)mAngles.vz;

		if (a < -0x800)
			a += 0x1000;

		if (a > 0x800)
			a -= 0x1000;

		if (a == 0)
			mAngVel.vz = 0;
		else
			mAngAcc.vz = a >> 3;
	}

	if (mApplyGravity)
		mAcc += *(CVector *)DAT_800a7f28;

	DoPhysics();

	if (mLoopingSoundUpdate != -1 && TTime % mLoopingSoundUpdate == 0)
		SFX_ModifyPos(mLoopingSound, mPos, 0);

	mTrodUpon = 0;
}
