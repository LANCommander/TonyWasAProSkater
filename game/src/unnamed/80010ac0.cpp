/* Crowd_Stop__Fv (0x80010ac0) */
#include "game/career.h"
#include "game/sfx.h"

struct A {
	int v;
	char pad[20];
};

extern SLevel Levels[];
extern int GLevel;
extern int GGame;
extern struct A Crowd[];

void Crowd_Stop() {
	int i;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			for (i = 0; i < 1; i++) {
				if (Crowd[i].v)
					SFX_Stop(Crowd[i].v);

				Crowd[i].v = 0;
			}
		}
	}
}
