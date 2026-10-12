/* Move__19CMotionBlurImmortal (0x80029bdc) */
/* C++ (BIT.CPP). Like CMotionBlur::Move, but shrinking and fading never kill the bit. */
#include "game/bit.h"

void CMotionBlurImmortal::Move() {
	mPos += mVel;

	if (!Shrink(0) && !Fade(0))
		IncFrameWithWrap();
}
