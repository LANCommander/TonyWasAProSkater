/* __8CQuadBit (0x80028394) */
/* C++ (BIT.CPP). A grey opaque textured quad, linked into QuadBitList. */
#include "game/bit.h"

extern CQuadBit *QuadBitList;

CQuadBit::CQuadBit() {
	mCodeBGR = 0x2c808080;
	AttachTo(&QuadBitList);
}
