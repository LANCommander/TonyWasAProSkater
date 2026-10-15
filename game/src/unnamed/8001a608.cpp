/* Front_SwitchOnVideoScreen__Fv (0x8001a608) */
#include "game/career.h"
#include "game/front.h"

extern int GVideoRestart[];
extern SLevel Levels[];
extern int GLevel;
extern int GGame;

extern int Status;

int Competition_IsOver();

void Front_SwitchOnVideoScreen() {
	int v;

	if (GVideoRestart[0] == 0 && Levels[GLevel].isCompetition != 0 && GGame == 1 &&
	    Competition_IsOver() == 0)
		v = 25;
	else {
		v = 16;

		if (GVideoRestart[0] == 0)
			v = 15;
	}

	Status = v;
	CommonSwitchOnMenuCode();
}
