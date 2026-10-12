/* SetSemiTransparent__8CQuadBit (0x80028498) */
/* C++ (BIT.CPP). Sets the semi-transparency bit of the primitive code. */
#include "game/bit.h"

void CQuadBit::SetSemiTransparent() {
	mCodeBGR |= 0x2000000;
}
