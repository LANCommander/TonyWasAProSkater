/* Fade__7CFT4Biti (0x80028290) */
/* C++ (BIT.CPP). Fades the RGB of the bit by mTransDecay per channel, clamping at 0. Returns 1 (and dies if Kill)
   once the colour has reached black, else 0. */
#include "game/bit.h"

int CFT4Bit::Fade(int Kill) {
	uint CodeBGR = mCodeBGR;
	int r, g, b;

	if ((CodeBGR & 0xffffff) == 0) {
		if (Kill)
			Die();

		return 1;
	}

	r = CodeBGR & 255;
	g = (CodeBGR >> 8) & 255;
	b = (CodeBGR >> 16) & 255;

	if (r < mTransDecay)
		r = 0;
	else
		r = (r - (uchar)mTransDecay) & 255;

	if (g < mTransDecay)
		g = 0;
	else
		g = (g - (uchar)mTransDecay) & 255;

	if (b < mTransDecay)
		b = 0;
	else
		b = (b - (uchar)mTransDecay) & 255;

	mCodeBGR = (CodeBGR & 0xff000000) | (b << 16) | (g << 8) | r;
	return 0;
}
