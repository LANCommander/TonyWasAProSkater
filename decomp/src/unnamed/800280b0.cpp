/* SetSemiTransparent__7CFT4Bit (0x800280b0) */
/* C++ (BIT.CPP). Sets the semi-transparency bit of the primitive code. */
#include "game/bit.h"

void CFT4Bit::SetSemiTransparent() {
	mCodeBGR |= 0x2000000;
}
