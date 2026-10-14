/* SetSemiTransparent__10CGPolyLine (0x800273a4) */
/* C++ (BIT2.CPP). Sets the semi-transparency bit in the GPU code of every segment and of the start. */
#include "game/bit.h"
#include "game/m3d.h"

void CGPolyLine::SetSemiTransparent() {
	for (int i = 0; i < mNumSegs; i++)
		mSegs[i].code |= 2;

	mCode |= 2;
}
