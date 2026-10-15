/* kickIt__Fii (0x8006b434) flags=-fstrict-aliasing */
#include "game/career.h"

extern SLevel Levels[];
extern char StartupGameState[];

extern int GLevel;

extern int SelectedSkater;

void Utils_CopyString(const char *a, char *b);
void LaunchGame();

void kickIt(int a, int b) {
	char *n;

	GLevel = a;
	n = Levels[a].pTRGName;
	SelectedSkater = b;
	Utils_CopyString(n, (StartupGameState + 4));

	StartupGameState[13] = 0;
	LaunchGame();
}
