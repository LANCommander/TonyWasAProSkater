/* GraffitiColor__FP6CBruce (0x8006ca1c) */
#include "names.h"

extern int GGame;
struct CBruce;

extern struct CBruce *GSkater;

void Mess_SetRGB(unsigned char r, unsigned char g, unsigned char b, int a);

void GraffitiColor(struct CBruce *a) {
	if (GGame == 8) {
		if (a == GSkater)
			Mess_SetRGB(80, 80, 128, 0);
		else
			Mess_SetRGB(128, 80, 80, 0);
	}
}
