/* IncFrameWithWrap__7CFT4Bit (0x800281c0) */
/* C++ (BIT.CPP). As IncFrame, but the frame wraps back to 0 once it passes the last one. */
#include "game/bit.h"
#include "game/m3d.h"

int TimeScale;

void CFT4Bit::IncFrameWithWrap() {
	short Frame = (mFrame << 8) | mFrameFrac;

	Frame += (mAnimSpeed * TimeScale) >> 8;
	mFrame = Frame >> 8;
	mFrameFrac = Frame;

	if (mFrame >= mNumFrames)
		mFrame = 0;

	mpPSXFrame = mpPSXAnim + mFrame;
}
