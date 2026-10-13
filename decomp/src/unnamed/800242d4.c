/* Game_Over__Fv (0x800242d4) */
#include "names.h"

typedef struct {
	char p[0x1f0];
	int f1f0;
	char q[0x838 - 0x1f4];
	int f838;
} B;

int GGame, GState, GSpin, GSpinSpeed;
B *GSkater;
B *DAT_800d17cc;

extern int GWins1, GLosses1, GStreak1, GHighTags1, GWins2, GLosses2, GStreak2, GHighTags2;

void Game_Over(void) {
	int a, b, c;

	if (GGame == 8) {
		a = DAT_800d17cc->f1f0;
		b = GSkater->f1f0;

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

			c = GSkater->f1f0;

			if (GHighTags1 < c)
				GHighTags1 = c;

			c = DAT_800d17cc->f1f0;

			if (GHighTags2 < c)
				GHighTags2 = c;
		}
	}

	if (GState == 0) {
		GState = 1;
		GSpin = 80;
		GSpinSpeed = 64;

		if (DAT_800d17cc)
			DAT_800d17cc->f838 = 0;
	}
}
