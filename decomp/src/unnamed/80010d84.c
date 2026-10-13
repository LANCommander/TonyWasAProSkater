/* FUN_80010d84 (0x80010d84) */
#include "names.h"

typedef struct {
	char p[0x4c];
	int f4c;
	char q[0x78 - 0x50];
} E;

extern E Levels[];
extern int GLevel;
extern int GGame;

int Crowd_Bored;
int DAT_800d10e8;

int SFX_PlayX(int, short, int);

void FUN_80010d84(int a) {
	int v;

	if (a) {
		if (Levels[GLevel].f4c) {
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
