/* Game_Over__Fv (0x800242d4) */
/* C++: the skaters are CBruce objects (their fields move in a 64-bit build) */
#include "game/bruce.h"

int GGame, GState, GSpin, GSpinSpeed;
CBruce *GSkater;
CBruce *DAT_800d17cc;

extern int GWins1, GLosses1, GStreak1, GHighTags1, GWins2, GLosses2, GStreak2, GHighTags2;

void Game_Over() {
	int a, b, c;

	if (GGame == 8) {
		a = DAT_800d17cc->mGraffitiCount;
		b = GSkater->mGraffitiCount;

		if (a != b) {
			if (b < a) {
				GWins2++;
				GLosses1++;

				if (GStreak2 < 0)
					GStreak2 = 1;
				else
					GStreak2++;

				GStreak1 = -GStreak2;
			} else {
				GWins1++;
				GLosses2++;

				if (GStreak1 < 0)
					GStreak1 = 1;
				else
					GStreak1++;

				GStreak2 = -GStreak1;
			}

			c = GSkater->mGraffitiCount;

			if (GHighTags1 < c)
				GHighTags1 = c;

			c = DAT_800d17cc->mGraffitiCount;

			if (GHighTags2 < c)
				GHighTags2 = c;
		}
	}

	if (GState == 0) {
		GState = 1;
		GSpin = 80;
		GSpinSpeed = 64;

		if (DAT_800d17cc)
			DAT_800d17cc->mLeanAngle = 0;
	}
}
