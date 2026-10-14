/* FlagOn__8CQuadBitUc (0x80028484) */
/* C++ (BIT.CPP). Sets the given flag bits. */
#include "game/bit.h"

void CQuadBit::FlagOn(uchar Flags) {
	mFlags |= Flags;
}
