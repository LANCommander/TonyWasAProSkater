/* Die__8CPowerUp (0x8001ff44) */
/* C++ (POWERUP.CPP). Kills the power-up once: marks it dead and hides it. */
#include "game/powerup.h"

void CPowerUp::Die() {
	if (!IsDead()) {
		mCBodyFlags |= 0x40;
		DontDisplay();
	}
}
