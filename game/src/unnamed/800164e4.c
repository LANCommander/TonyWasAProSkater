/* InCompetition__Fv (0x800164e4) */
#include "names.h"

struct T {
	char pad[12];
	int f;
	char q[104];
};

extern struct T Levels[];
extern int GLevel;
extern int GGame;

int InCompetition(void) {
	if (Levels[GLevel].f != 0 && GGame == 1)
		return 1;

	return 0;
}
