/* SetUVs__9CChunkBitUsUsUcUcUcUcUcUc (0x80028b1c) */
/* C++ (BIT.CPP). Sets the chunk's CLUT, texture page and the UVs of three corners. */
#include "game/bit.h"

void CChunkBit::SetUVs(ushort clut, ushort tpage, uchar u0, uchar v0, uchar u1, uchar v1, uchar u2,
                       uchar v2) {
	mClutV0U0 = (clut << 16) | (v0 << 8) | u0;
	mTPageV1U1 = (tpage << 16) | (v1 << 8) | u1;
	mV2U2 = (v2 << 8) | u2;
}
