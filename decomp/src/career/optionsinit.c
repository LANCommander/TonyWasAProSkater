/* Options_Init__Fv (0x800105f4) */
#include "names.h"

extern int GPreferences[];
extern int SFXLEVEL, XALEVEL;

int FUN_8008b900(char *, char *);

void Options_Init(void) {
	int i;

	GPreferences[0] = 12;
	SFXLEVEL = 16380;
	GPreferences[1] = 6;
	XALEVEL = 8190;
	GPreferences[97] = 1;
	GPreferences[98] = 1;

	for (i = 0; i < 2; i++) {
		GPreferences[68 + i] = 1;
		GPreferences[70 + i] = 1;
		GPreferences[72 + i * 4] = 0;
		GPreferences[73 + i * 4] = 1;
		GPreferences[74 + i * 4] = 2;
		GPreferences[75 + i * 4] = 3;
	}

	FUN_8008b900((char *)&GPreferences[100], "HORSE");

	GPreferences[99] = 5;
	FUN_8008b900((char *)&GPreferences[100] - 80, "AAA");
}
