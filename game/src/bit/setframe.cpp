/* SetFrame__7CFT4Biti (0x80028140) */
/* C++ (BIT.CPP). Jumps the animation to the given frame. */
#include "game/bit.h"
#include "game/m3d.h"

void CFT4Bit::SetFrame(int Frame) {
	mFrame = Frame;
	mFrameFrac = 0;
	mpPSXFrame = mpPSXAnim + mFrame;
}
