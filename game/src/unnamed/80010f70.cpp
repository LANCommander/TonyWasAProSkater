/* Crowd_Ahh__Fi (0x80010f70) */
#include "game/career.h"
#include "game/sfx.h"

extern SLevel Levels[];
extern int GLevel;
extern int GGame;

extern int Crowd_Bored;

void Crowd_Ahh(int a) {
	int s;

	if (Levels[GLevel].Crowd && GGame == 1) {
		if (a < 100)
			Crowd_Bored += 2;
		else
			Crowd_Bored = 0;

		s = (a << 14) / 1000;

		if (s > 16384)
			s = 16384;

		if (s < 4096)
			s = 4096;
		else
			SFX_PlayX(46, (short)(s / 2), 0);

		SFX_PlayX(47, (short)s, 0);
	}
}
