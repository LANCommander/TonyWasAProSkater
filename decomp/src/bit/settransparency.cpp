/* SetTransparency__8CQuadBitUc (0x800284ac) */
/* C++ (BIT.CPP). Sets R, G and B to the same level, keeping the primitive code byte. */
#include "game/bit.h"

void CQuadBit::SetTransparency(uchar Level) {
	mCodeBGR = (mCodeBGR & 0xff000000) | (Level << 16) | (Level << 8) | Level;
}
