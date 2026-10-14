/* Career_GiveTape__Fii (0x80010724) */
/* C++ (CAREER.CPP). Demo twin; locals from its MAIN.SYM. Marks a level goal (tape) as got for the
   current skater, recounts the skater's goals over the ten levels that count, and bumps the
   skater's goal-got counter. */
#include "game/career.h"
#include "game/bruce.h"

extern CBruce *GSkater;
extern SLevel Levels[];
extern SCareer GCareer;

void Career_GiveTape(int Level, int Goal) {
	int t = 0;

	GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[Level] |= 1 << Goal;

	{
		int j;

		for (j = 0; j < 10; j++) {
			int i;

			for (i = 0; i < 5; i++) {
				if (!Levels[j].isCompetition &&
				    ((GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[j] >> i) & 1))
					t++;
			}
		}
	}

	GCareer.CharacterProgress[GSkater->mSkaterIndex].NumTapes = t;
	GSkater->mNumTapes++;
}
