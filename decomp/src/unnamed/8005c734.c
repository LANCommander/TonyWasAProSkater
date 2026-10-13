/* Front_SetMusicLoop__Fi (0x8005c734) */
#include "names.h"

extern int GPreferences_4[];

int DAT_800d1cbc;
int DAT_800d1cc0;
int DAT_800d1cc4;

void Front_SetMusicLoop(int a) {
	DAT_800d1cc0 = GPreferences_4[0] * 16383 / 12;

	if (DAT_800d1cbc != a) {
		DAT_800d1cbc = a;
		DAT_800d1cc4 = 1;
	}
}
