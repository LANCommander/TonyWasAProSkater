/* Competition_WonGameAllGold__Fv (0x8007401c) */
/* C++. Whether the skater has a gold medal (bit 5 of the level's career byte) in all three
   competition levels. */
#include "game/bruce.h"
#include "game/career.h"

extern SLevel Levels[];
extern SCareer GCareer;
extern CBruce *GSkater;

int Competition_WonGameAllGold() {
	int c = 0;
	int i = 0;
	CBruce *g = GSkater;

	for (; i < 10; i++) {
		if (Levels[i].isCompetition) {
			if (GCareer.CharacterProgress[0].Tapes[i + g->mSkaterIndex * 14] & 32)
				c++;
		}
	}

	return c == 3;
}
