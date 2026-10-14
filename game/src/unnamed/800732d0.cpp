/* CreateBit__5CTape (0x800732d0) */
/* C++ (TAPES.CPP). Makes new bits come from the bit cache. */
#include "game/tapes.h"

extern int UseCacheForBits;

void CTape::CreateBit() {
	UseCacheForBits = 1;
}
