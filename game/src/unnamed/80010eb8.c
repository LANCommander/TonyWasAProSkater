/* Crowd_Oww__Fi (0x80010eb8) */
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

int SFX_PlayX(int, short, int);

void Crowd_Oww(int Speed) {
	int Volume;

	if (Levels[GLevel].f4c) {
		if (GGame == 1) {
			Volume = (Speed << 14) / 80;
			Crowd_Bored++;

			if (Volume > 16384)
				Volume = 16384;

			if (Volume < 2048)
				Volume = 2048;

			SFX_PlayX(49, Volume, 0);
		}
	}
}
