/* Shrink__7CFT4Biti (0x80028234) */
/* C++ (BIT.CPP). Shrinks the bit by mScaleDecay, clamping at 0. Returns 1 (and dies if Kill) once the scale
   is already 0, else 0. */
#include "game/bit.h"

int CFT4Bit::Shrink(int Kill) {
	if (mScale == 0) {
		if (Kill)
			Die();

		return 1;
	}

	mScale -= mScaleDecay;

	if (mScale < 0)
		mScale = 0;

	return 0;
}
