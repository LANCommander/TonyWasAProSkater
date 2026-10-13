/* Shatter_MaybeMakeGlassShatterSound__Fv (0x80022440) */
#include "names.h"

int ShatterSound;

extern int GLevel;

void SFX_PlayX(int, int, int);

void Shatter_MaybeMakeGlassShatterSound(void) {
	int s;

	if (ShatterSound) {
		if (GLevel == 1)
			s = 0x3a;
		else {
			s = 0x3a;

			if (GLevel != 4)
				s = 0x2a;
		}

		SFX_PlayX(s, 0x2000, 0);
	}

	ShatterSound = 0;
}
