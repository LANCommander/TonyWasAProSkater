/* Front_SwitchOnVideoScreen__Fv (0x8001a608) */
#include "names.h"

struct T {
	char pad[12];
	int f;
	char q[104];
};

extern int GVideoRestart[];
extern struct T Levels[];
extern int GLevel;
extern int GGame;

int Status;

int Competition_IsOver(void);
void CommonSwitchOnMenuCode(void);

void Front_SwitchOnVideoScreen(void) {
	int v;

	if (GVideoRestart[0] == 0 && Levels[GLevel].f != 0 && GGame == 1 && Competition_IsOver() == 0)
		v = 25;
	else {
		v = 16;

		if (GVideoRestart[0] == 0)
			v = 15;
	}

	Status = v;
	CommonSwitchOnMenuCode();
}
