/* SetGravity__8CPowerUpii (0x8001feb8) */
/* C++ (POWERUP.CPP). Sets the power-up's vertical acceleration and vertical friction. */
#include "game/powerup.h"

void CPowerUp::SetGravity(int g, int fric) {
	mAcc.vy = g;
	mFric.vy = fric;
}
