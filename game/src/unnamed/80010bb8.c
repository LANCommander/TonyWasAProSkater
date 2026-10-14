/* Crowd_Update__Fv (0x80010bb8) */
#include "names.h"

typedef struct {
	int a0, a1, a2, a3, a4, a5;
} S;

typedef struct {
	char p[0x4c];
	int f4c;
	char q[0x78 - 0x50];
} E;

extern E Levels[];
extern S Crowd[];
extern int GLevel;
extern int GGame;

int Crowd_Bored;

int Crowd_MoveTowards(int *, int, int);
void FUN_80011060(int);
void SFX_ModifyVol(int, int, int);
int Redbook_Rnd(int);

void Crowd_Update(void) {
	int i;

	if (Levels[GLevel].f4c) {
		if (GGame == 1) {
			for (i = 0; i <= 0; i++) {
				if (Crowd[i].a0) {
					if (Crowd_MoveTowards(&Crowd[i].a1, Crowd[i].a3, 100) ||
					    Crowd_MoveTowards(&Crowd[i].a2, Crowd[i].a4, 100)) {
						SFX_ModifyVol(Crowd[i].a0, (Crowd[i].a1 * 2) / 3, (Crowd[i].a2 * 2) / 3);
					} else {
						if (Redbook_Rnd(30) == 0) {
							Crowd[i].a3 = Redbook_Rnd(8192);
							Crowd[i].a4 = Redbook_Rnd(8192);
						}
					}
				}
			}

			if (Crowd_Bored && Redbook_Rnd(50) == 0) {
				FUN_80011060(Crowd_Bored << 13);

				Crowd_Bored--;
			}
		}
	}
}
