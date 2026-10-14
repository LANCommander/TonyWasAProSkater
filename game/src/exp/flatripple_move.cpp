/* Move__11CFlatRipple (0x8004ebf4) */
#include "game/bit.h"
#include "game/m3d.h"
#include "game/trig.h"

extern SSinCos rcossin_tbl[2];

void CFlatRipple::Move() {
	int intensity;
	int a;
	int step;

	mRadius += mGrowRate;

	if (mRadius >= mMaxRadius) {
		mRadius = mMaxRadius;
		Die();
	}

	intensity = ((mMaxRadius - mRadius) << 8) / mMaxRadius;
	a = mAngle;
	step = 4096 / mNumSegs;
	SSinCos *e = &rcossin_tbl[a & 4095];

	mStart.vx = mPos.vx + (((mRadius * e->c) / 4096) << 12);
	mStart.vz = mPos.vz + (((mRadius * e->s) / 4096) << 12);

	for (int i = 0; i < mNumSegs; i++) {
		mSegs[i].r = (mR * intensity) >> 8;
		mSegs[i].g = (mG * intensity) >> 8;
		mSegs[i].b = (mB * intensity) >> 8;
		a += step;

		if (i == mNumSegs - 1) {
			mSegs[i].End = mStart;
		} else {
			SSinCos *f = &rcossin_tbl[a & 4095];

			mSegs[i].End.vx = mPos.vx + (((mRadius * f->c) / 4096) << 12);
			mSegs[i].End.vz = mPos.vz + (((mRadius * f->s) / 4096) << 12);
		}
	}
}
