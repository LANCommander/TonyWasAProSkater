/* Crowd_Cheer__Fi (0x80010d84) */
#include "game/career.h"
#include "game/sfx.h"

extern SLevel Levels[];
extern int GLevel;
extern int GGame;

extern int Crowd_Bored;
extern int DAT_800d10e8;

void Crowd_Cheer(int a) {
	int v;

	if (a) {
		if (Levels[GLevel].Crowd) {
			if (GGame == 1) {
				if (Crowd_Bored) {
					if (a >= 100)
						Crowd_Bored = 0;
					else
						Crowd_Bored--;
				}

				v = (a << 14) / 1000;

				if (v > 16384)
					v = 16384;

				DAT_800d10e8 ^= 1;

				if (v > 2048) {
					SFX_PlayX(48, v, 0);
					SFX_PlayX(46, v, 0);

					if (a >= 8000)
						SFX_PlayX(49, v, 0);
				} else {
					if (a >= 400)
						SFX_PlayX(46, v * 4, 0);
				}
			}
		}
	}
}
