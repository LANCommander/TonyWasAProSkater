/* GlowUpdate__6CBruce (0x800424b4) */
/* C++ (PHYSICS.CPP). Fades the skater's glow towards its target colour; once there, fades back to
   black at the down speed. A lit glow shows as the body's ellipsoid. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

int GlowTowards(int, int, int);

void CBruce::GlowUpdate() {
	mGlowR = GlowTowards(mGlowR, mTargetGlowR, mGlowSpeed);
	mGlowG = GlowTowards(mGlowG, mTargetGlowG, mGlowSpeed);
	mGlowB = GlowTowards(mGlowB, mTargetGlowB, mGlowSpeed);

	if (mGlowR == mTargetGlowR && mGlowG == mTargetGlowG && mGlowB == mTargetGlowB) {
		if (mGlowDownSpeed) {
			mTargetGlowR = 0;
			mTargetGlowG = 0;
			mTargetGlowB = 0;
			mGlowSpeed = mGlowDownSpeed;
			mGlowDownSpeed = 0;
		}
	}

	if (mGlowR | mGlowG | mGlowB) {
		mEllipsoidScale = 0x2000;
		mEllipsoidOTPushback = 10;
		mEllipsoidRGB = 0x2000000 | mGlowR | (mGlowG << 8) | (mGlowB << 16);
	} else {
		mEllipsoidRGB = 0;
	}
}
