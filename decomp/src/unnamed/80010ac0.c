/* Crowd_Stop__Fv (0x80010ac0) */
#include "names.h"

struct S {
	char pad[76];
	int f;
	char q[40];
};

struct A {
	int v;
	char pad[20];
};

extern struct S Levels[];
extern int GLevel;
extern int GGame;
extern struct A Crowd[];

void SFX_Stop(unsigned a);

void Crowd_Stop(void) {
	int i;

	if (Levels[GLevel].f) {
		if (GGame == 1) {
			for (i = 0; i < 1; i++) {
				if (Crowd[i].v)
					SFX_Stop(Crowd[i].v);

				Crowd[i].v = 0;
			}
		}
	}
}
