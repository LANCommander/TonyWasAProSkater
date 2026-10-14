/* IncFrame__7CFT4Bit (0x80028160) */
/* C++ (BIT.CPP). Advances the 8.8 fixed-point animation frame by mAnimSpeed (scaled by TimeScale) and
   points mpPSXFrame at the new frame. No wrapping. */
#include "game/bit.h"
#include "game/m3d.h"

int TimeScale;

void CFT4Bit::IncFrame() {
	short Frame = (mFrame << 8) | mFrameFrac;

	Frame += (mAnimSpeed * TimeScale) >> 8;
	mFrame = Frame >> 8;
	mFrameFrac = Frame;
	mpPSXFrame = mpPSXAnim + mFrame;
}
