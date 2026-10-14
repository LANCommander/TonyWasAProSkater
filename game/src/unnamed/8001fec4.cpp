/* CreateBit__8CPowerUp (0x8001fec4) */
/* C++ (POWERUP.CPP). Makes the next bits come from the bit cache. */
#include "game/powerup.h"

int UseCacheForBits;

void CPowerUp::CreateBit() {
	UseCacheForBits = 1;
}
