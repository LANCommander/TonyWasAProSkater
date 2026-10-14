/* SetSemiTransparent__9CPolyLine (0x80027210) */
/* C++ (BIT2.CPP). Sets the semi-transparency bit in the GPU code of every segment. */
#include "game/bit.h"
#include "game/m3d.h"

void CPolyLine::SetSemiTransparent() {
	for (int i = 0; i < mNumSegs; i++)
		mSegs[i].code |= 2;
}
