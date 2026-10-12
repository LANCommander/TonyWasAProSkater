/* __8CFlatBit (0x8002930c) */
/* C++ (BIT.CPP). A screen-facing sprite, linked into FlatBitList: angular friction 1, post-scale
   1.0 x 1.0, semi-transparency rate 32. */
#include "game/bit.h"

CFlatBit *FlatBitList;

CFlatBit::CFlatBit() {
	AttachTo(&FlatBitList);

	mSemiTransparencyRate = 32;
	mAngFric = 1;
	mPostScale = 0x10001000;
}
