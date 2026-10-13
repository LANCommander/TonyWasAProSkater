/* FUN_80060854 (0x80060854) */
#include "names.h"

typedef struct {
	unsigned char a, b, c, d, e[10];
} R;

extern R GCareer_8[];
extern int Cheat_LevelSelect;

int DAT_800d1d7c, DAT_800d1d80;

int Front_RunMenu(char **, int *, int, int, int *, int, int, int, int);
void printf(char *);

int FUN_80060854(int a) {
	char *l[2] = { "GET ALL TAPES", "UNLOCK OR LOCK LEVELS" };
	int k[2];
	int v, i, j;

	k[0] = 0;
	k[1] = 0;
	v = Front_RunMenu(l, k, 2, 112, &a, 0, 0, 0, 0);

	if (v == 113) {
		DAT_800d1d7c = 1;
		DAT_800d1d80 = 1;

		for (i = 0; i < 11; i++) {
			GCareer_8[i].a = 0;
			GCareer_8[i].d = 0;
			GCareer_8[i].c = 0;
			GCareer_8[i].b = 0;

			for (j = 0; j < 10; j++) {
				GCareer_8[i].e[j] = 255;
				GCareer_8[i].a += 5;
				GCareer_8[i].d += 1;
				GCareer_8[i].c += 1;
				GCareer_8[i].b += 1;
			}
		}
	} else if (v == 114) {
		Cheat_LevelSelect = Cheat_LevelSelect ^ 1;

		if (Cheat_LevelSelect == 0)
			printf("\nLevels locked");
		else
			printf("\nLevels unlocked");
	}

	return -1;
}
