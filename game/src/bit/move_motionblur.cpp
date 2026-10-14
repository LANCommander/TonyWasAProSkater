/* Move__11CMotionBlur (0x80029b2c) */
/* C++ (BIT.CPP). The blur moves with its velocity, shrinks and fades (dying when either runs out)
   and otherwise steps its animation. */
#include "game/bit.h"

void CMotionBlur::Move() {
	mPos += mVel;

	if (!Shrink(1) && !Fade(1))
		IncFrameWithWrap();
}
