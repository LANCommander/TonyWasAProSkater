/* DontDisplay__8CPowerUp (0x8001fe7c) */
/* C++ (POWERUP.CPP). Hides the power-up and frees its extras (virtual DeleteStuff). */
#include "game/powerup.h"

void CPowerUp::DontDisplay() {
	mFlags |= 0x41;
	DeleteStuff();
}
