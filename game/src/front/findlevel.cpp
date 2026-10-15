/* Front_FindLevel__FPc (0x8001cb30) */
#include "game/career.h"

extern SLevel Levels[];
extern int CurrentGameLevel;

extern int Utils_CompareStrings(const char *, const char *, uint);

SLevel *Front_FindLevel(char *s) {
	SLevel *r = 0;
	int i = 0;

	while (*Levels[i].pTRGName) {
		if (Utils_CompareStrings(Levels[i].pTRGName, s, 0xffffffff)) {
			CurrentGameLevel = i;
			r = &Levels[i];
			break;
		}

		if (++i >= 41)
			break;
	}

	return r;
}
