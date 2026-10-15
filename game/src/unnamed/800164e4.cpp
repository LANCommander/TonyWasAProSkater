/* InCompetition__Fv (0x800164e4) */
#include "game/career.h"

extern SLevel Levels[];
extern int GLevel;
extern int GGame;

int InCompetition() {
	if (Levels[GLevel].isCompetition != 0 && GGame == 1)
		return 1;

	return 0;
}
