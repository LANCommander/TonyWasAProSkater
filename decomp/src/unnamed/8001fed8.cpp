/* DeleteStuff__8CPowerUp (0x8001fed8) */
/* C++ (POWERUP.CPP). Frees the power-up's shadow and glow. */
#include "game/powerup.h"
#include "game/bit.h"

void CPowerUp::DeleteStuff() {
	KillShadow();

	if (mpGlow)
		delete mpGlow;

	mpGlow = 0;
}
