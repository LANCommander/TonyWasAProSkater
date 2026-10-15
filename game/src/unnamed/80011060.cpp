/* FUN_80011060 (0x80011060) */
#include "game/career.h"
#include "game/redbook.h"
#include "game/sfx.h"

extern SLevel Levels[];
extern int GLevel;
extern int GGame;


extern "C" void FUN_80011060(int a) {
	short s1, s0;

	if (Levels[GLevel].Crowd && GGame == 1) {
		s1 = (a * (Redbook_Rnd(20) + 90)) / 100;
		s0 = (a * (Redbook_Rnd(20) + 90)) / 100;
		SFX_PlayLR(50, s1, s0, 0);
	}
}
