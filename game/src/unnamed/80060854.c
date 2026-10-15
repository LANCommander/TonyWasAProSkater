/* FUN_80060854 (0x80060854) */
#include "names.h"

typedef struct {
	unsigned char a, b, c, d, e[10];
} R;

extern unsigned char GCareer[];
#define CharacterProgress ((R *)(GCareer + 8)) /* GCareer.CharacterProgress */
extern int Cheat_LevelSelect;

extern int DAT_800d1d7c, DAT_800d1d80;

struct VerticalMenu;

int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);
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
			CharacterProgress[i].a = 0;
			CharacterProgress[i].d = 0;
			CharacterProgress[i].c = 0;
			CharacterProgress[i].b = 0;

			for (j = 0; j < 10; j++) {
				CharacterProgress[i].e[j] = 255;
				CharacterProgress[i].a += 5;
				CharacterProgress[i].d += 1;
				CharacterProgress[i].c += 1;
				CharacterProgress[i].b += 1;
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
