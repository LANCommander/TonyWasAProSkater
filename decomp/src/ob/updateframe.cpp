/* UpdateFrame__6CSuper (0x80043f70) */
/* C++ (OB.CPP). Advances the animation frame by mAnimSpeed (scaled by TimeScale) in 16.16 fixed
   point in direction mAnimDir. Mode 0 (play to target) clamps at mTargetFrame, ping-ponging to
   mTargetFrame2 when one is set and flagging mAnimFinished otherwise; mode 1 (cycle) wraps the
   frame within mNumFrames. */
#include "game/ob.h"

extern int TimeScale;

void CSuper::UpdateFrame() {
	int v, a, c, d, s;
	unsigned char ub;

	if (mAnimMode == 0) {
		c = mAnimDir;

		if ((c == 1 && (a = (unsigned char)mTargetFrame, mFrame >= mTargetFrame)) ||
		    (c == -1 && (a = (unsigned char)mTargetFrame, mFrame <= mTargetFrame))) {
			if ((ub = mTargetFrame2, mTargetFrame2 > 0)) {
				mTargetFrame = ub;
				mTargetFrame2 = a;
				mAnimDir = -(unsigned char)mAnimDir;
			} else
				mAnimFinished = 1;
		}
	}

	v = (mFrame << 16) | mFrameFrac;
	d = mAnimDir;
	a = (mAnimSpeed * TimeScale) >> 8;

	if (d == 1)
		v += a;

	if (d == -1)
		v -= a;

	s = v >> 16;
	mFrame = s;
	mFrameFrac = v;

	switch ((unsigned char)mAnimMode) {
		case 0:
			if ((d == 1 && !(s < mTargetFrame)) || (d == -1 && !(mTargetFrame < s)))
				mFrame = (signed char)(unsigned char)mTargetFrame;

			break;
		case 1:
			if (s >= mNumFrames)
				mFrame = 0;

			if (mFrame < 0)
				mFrame = mNumFrames - 1;

			break;
	}
}
