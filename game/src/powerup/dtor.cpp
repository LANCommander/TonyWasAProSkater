/* _._8CPowerUp (0x8001fe14) */
/* C++ (POWERUP.CPP). Leaves the power-up list and frees the extras; CBody's destructor does the
   rest. */
#include "game/powerup.h"

extern CBody *PowerUpList;

CPowerUp::~CPowerUp() {
	DeleteFrom(&PowerUpList);
	CPowerUp::DeleteStuff();
}
