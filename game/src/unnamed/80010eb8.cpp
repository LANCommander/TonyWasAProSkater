/* Crowd_Oww__Fi (0x80010eb8) */
#include "game/career.h"
#include "game/sfx.h"

extern SLevel Levels[];
extern int GLevel;
extern int GGame;

extern int Crowd_Bored;

void Crowd_Oww(int Speed) {
	int Volume;

	if (Levels[GLevel].Crowd) {
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
