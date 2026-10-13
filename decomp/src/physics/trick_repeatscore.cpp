/* Trick_RepeatScore__6CBruceii (0x8002f290) */
/* C++ (PHYSICS.CPP). Depreciates a trick's score by how often it has been done (the per-trick
   count m428[Trick]): 0-1 times full, 2 times 75%, 3 times 1/2, 4 times 1/4, otherwise 1/10. */
#include "game/bruce.h"

int CBruce::Trick_RepeatScore(int Trick, int Score) {
	switch (m428[Trick]) {
		case 0:
		case 1:
			break;
		case 2:
			Score = Score * 75 / 100;
			break;
		case 3:
			Score /= 2;
			break;
		case 4:
			Score /= 4;
			break;
		default:
			Score /= 10;
			break;
	}

	return Score;
}
