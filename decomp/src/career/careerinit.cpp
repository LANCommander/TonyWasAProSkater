/* Career_Init__Fv (0x80010000) */
/* C++ (CAREER.CPP). Demo twin; locals from its MAIN.SYM. Clears the career save, stamps its
   version, then fills every level's high-score table with random names from the name list,
   scores stepping down 5000 from the level's base and random skaters. THPS1 has 20 levels and
   no Career_Test call. */
#include "game/career.h"

extern SCareer GCareer;
extern char *DefaultHighScoreInitials[];
extern SLevel Levels[];
extern int GHasCareerLoaded, GLoadedCareerIsSaved;

int Redbook_Rnd(int);
void Options_SetPreferences();

void Career_Init() {
	char *p;
	char **pHighScore;
	int HighScoreCount;

	{
		int i;

		p = (char *)&GCareer;

		for (i = 0; i < sizeof(SCareer); i++)
			*p++ = 0;
	}

	GCareer.Version = 0x60299;
	pHighScore = DefaultHighScoreInitials;
	HighScoreCount = 0;

	while (*pHighScore) {
		pHighScore++;
		HighScoreCount++;
	}

	{
		int Level;

		for (Level = 0; Level < 20; Level++) {
			int Score;

			for (Score = 0; Score < 8; Score++) {
				pHighScore = &DefaultHighScoreInitials[Redbook_Rnd(HighScoreCount)];
				GCareer.HighScore[Level][Score].Name[0] = (*pHighScore)[0];
				GCareer.HighScore[Level][Score].Name[1] = (*pHighScore)[1];
				GCareer.HighScore[Level][Score].Name[2] = (*pHighScore)[2];
				GCareer.HighScore[Level][Score].Score =
				    Levels[Level].HiScore + (7 - Score) * 5000;
				GCareer.HighScore[Level][Score].Skater = Redbook_Rnd(10);
			}
		}
	}

	GHasCareerLoaded = 0;
	GLoadedCareerIsSaved = 0;

	{
		int i;

		for (i = 0; i < 2; i++)
			GCareer.LastTrucks[i] = 1;
	}

	Options_SetPreferences();
}
