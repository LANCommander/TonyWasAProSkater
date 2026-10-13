/* FUN_80010f70 (0x80010f70) */
#include "names.h"

struct S {
	char pad[76];
	int f;
	char q[40];
};

extern struct S Levels[];
extern int GLevel;
extern int GGame;

int Crowd_Bored;

void SFX_PlayX(int a, int b, int c);

void FUN_80010f70(int a) {
	int s;

	if (Levels[GLevel].f && GGame == 1) {
		if (a < 100)
			Crowd_Bored += 2;
		else
			Crowd_Bored = 0;

		s = (a << 14) / 1000;

		if (s > 16384)
			s = 16384;

		if (s < 4096)
			s = 4096;
		else
			SFX_PlayX(46, (short)(s / 2), 0);

		SFX_PlayX(47, (short)s, 0);
	}
}
