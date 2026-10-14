/* SetNode__8CPowerUpi (0x8001ff34) */
/* C++ (POWERUP.CPP). Records the power-up's node index and marks it as having one. */
#include "game/powerup.h"

void CPowerUp::SetNode(int Node) {
	mNodeIndex = Node;
	mHasNode = 1;
}
