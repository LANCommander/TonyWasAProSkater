/* LevelSelect_Run__Fi (0x8006658c) */
#include "names.h"

int DAT_800d2368;
int DAT_800d1dc4;

void Mess_SetCurrentFont(char *p);
void Mess_SetRGB(unsigned char r, unsigned char g, unsigned char b, int a);
void Mess_SetFontScale(unsigned a);
void Mess_SetTextJustify(int a);
void Mess_SetFontShadowOffset(int a);
void LevelSelect_Screen(void);

int LevelSelect_Run(int a) {
	DAT_800d2368 = a;
	Mess_SetCurrentFont("mainf.fnt");
	Mess_SetRGB(128, 128, 128, 0);
	Mess_SetFontScale(256);
	Mess_SetTextJustify(1);
	Mess_SetFontShadowOffset(4);
	LevelSelect_Screen();
	return DAT_800d1dc4;
}
