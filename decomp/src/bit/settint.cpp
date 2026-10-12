/* SetTint__7CFT4BitUcUcUc (0x800280c4) */
/* C++ (BIT.CPP). Sets the RGB of the bit, keeping the primitive code byte. */
#include "game/bit.h"

void CFT4Bit::SetTint(uchar r, uchar g, uchar b) {
	mCodeBGR = (mCodeBGR & 0xff000000) | (b << 16) | (g << 8) | r;
}
