/* Career_GiveMedal__Fi10EMedalType (0x8001081c) */
/* C++ (CAREER.CPP). Demo twin; locals from its MAIN.SYM. Records a competition medal for the
   current skater on that level (bits 5-7 of the level byte: gold, silver, bronze) and recounts
   the skater's medals over the ten levels that count. */
#include "game/career.h"
#include "game/bruce.h"

enum EMedalType { MEDAL_NONE };

extern CBruce *GSkater;
extern SLevel Levels[];
extern SCareer GCareer;

void Career_GiveMedal(int Level, EMedalType medal) {
	int g = 0;
	int s = 0;
	int b = 0;

	GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[Level] |= 1 << medal;

	{
		int i;

		for (i = 0; i < 10; i++) {
			if (!Levels[i].isCompetition) {
				uchar t = GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[i];

				if (t & 0x20)
					g++;

				if (t & 0x40)
					s++;

				if (t & 0x80)
					b++;
			}
		}
	}

	GCareer.CharacterProgress[GSkater->mSkaterIndex].NumGold = g;
	GCareer.CharacterProgress[GSkater->mSkaterIndex].NumSilver = s;
	GCareer.CharacterProgress[GSkater->mSkaterIndex].NumBronze = b;
}
