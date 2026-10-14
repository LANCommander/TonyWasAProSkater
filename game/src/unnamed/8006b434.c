/* kickIt__Fii (0x8006b434) flags=-fstrict-aliasing */
#include "names.h"

struct T {
	char pad[20];
	char *n;
	char q[96];
};

extern struct T Levels[];
extern char DAT_800a5c68[];

int GLevel;

extern int SelectedSkater;

void Utils_CopyString(char *a, char *b);
void LaunchGame(void);

void kickIt(int a, int b) {
	char *n;

	GLevel = a;
	n = Levels[a].n;
	SelectedSkater = b;
	Utils_CopyString(n, DAT_800a5c68);

	DAT_800a5c68[9] = 0;
	LaunchGame();
}
