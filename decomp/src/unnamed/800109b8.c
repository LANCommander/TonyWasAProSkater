/* FUN_800109b8 (0x800109b8) */
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

int SFX_PlayX(int, int, int);
int SFX_GetVoicePitch(int);
void FUN_8008c190(char *, int);

void FUN_800109b8(void) {
	int i;

	if (Levels[GLevel].f4c) {
		if (GGame == 1) {
			Crowd_Bored = 0;

			for (i = 0; i < 1; i++) {
				Crowd[i].a0 = SFX_PlayX(44, 0, 0);
				FUN_8008c190("Pitch = %d \n", SFX_GetVoicePitch(Crowd[i].a0));

				Crowd[i].a1 = Crowd[i].a3 = Crowd[i].a4 = Crowd[i].a2 = 0;
			}
		}
	}
}
