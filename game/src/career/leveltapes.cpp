/* Career_LevelTapes__Fi (0x80010944) */
/* C++ (CAREER.CPP). Demo twin; locals from its MAIN.SYM. Counts the current skater's tapes (goals)
   on the current level; THPS1 ignores the Level argument. */
#include "game/career.h"
#include "game/bruce.h"

extern CBruce *GSkater;
extern SCareer GCareer;
extern int GLevel;

int Career_LevelTapes(int Level) {
	int t = 0;

	{
		int i;

		for (i = 0; i < 5; i++)
			if ((GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[GLevel] >> i) & 1)
				t++;
	}

	return t;
}
