/* Front_SetMusicLoop__Fi (0x8005c734) */
#include "names.h"

extern int GPreferences[];

extern int DAT_800d1cbc;
extern int DAT_800d1cc0;
extern int DAT_800d1cc4;

void Front_SetMusicLoop(int a) {
	DAT_800d1cc0 = GPreferences[1] * 16383 / 12;

	if (DAT_800d1cbc != a) {
		DAT_800d1cbc = a;
		DAT_800d1cc4 = 1;
	}
}
