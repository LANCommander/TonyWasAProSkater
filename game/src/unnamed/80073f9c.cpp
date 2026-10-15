/* FUN_80073f9c (0x80073f9c) */
/* C++: whether the current skater has a medal on all three competition levels. */
#include "game/bruce.h"
#include "game/career.h"

extern SLevel Levels[];
extern SCareer GCareer;
extern CBruce *GSkater;

extern "C" int FUN_80073f9c(void) {
	int c = 0;
	int i = 0;
	CBruce *g = GSkater;

	for (; i < 10; i++) {
		if (Levels[i].isCompetition) {
			if (GCareer.CharacterProgress[g->mSkaterIndex].Tapes[i] & 224)
				c++;
		}
	}

	return c == 3;
}
