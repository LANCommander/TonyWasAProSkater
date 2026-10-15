/* Crowd_Start__Fv (0x800109b8) */
#include "game/career.h"
#include "game/psyq.h"
#include "game/sfx.h"

typedef struct {
	int a0, a1, a2, a3, a4, a5;
} S;

extern SLevel Levels[];
extern S Crowd[];
extern int GLevel;
extern int GGame;

extern int Crowd_Bored;

int SFX_GetVoicePitch(uint);

void Crowd_Start() {
	int i;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			Crowd_Bored = 0;

			for (i = 0; i < 1; i++) {
				Crowd[i].a0 = SFX_PlayX(44, 0, 0);
				printf("Pitch = %d \n", SFX_GetVoicePitch(Crowd[i].a0));

				Crowd[i].a1 = Crowd[i].a3 = Crowd[i].a4 = Crowd[i].a2 = 0;
			}
		}
	}
}
