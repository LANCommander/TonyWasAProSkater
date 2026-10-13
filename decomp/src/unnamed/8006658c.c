/* FUN_8006658c (0x8006658c) */
#include "names.h"

int DAT_800d2368;
int DAT_800d1dc4;

void Mess_SetCurrentFont(char *p);
void Mess_SetRGB(unsigned char r, unsigned char g, unsigned char b, int a);
void Mess_SetFontScale(unsigned a);
void Mess_SetTextJustify(int a);
void FUN_8001f1b4(int a);
void FUN_80066d7c(void);

int FUN_8006658c(int a) {
	DAT_800d2368 = a;
	Mess_SetCurrentFont("mainf.fnt");
	Mess_SetRGB(128, 128, 128, 0);
	Mess_SetFontScale(256);
	Mess_SetTextJustify(1);
	FUN_8001f1b4(4);
	FUN_80066d7c();
	return DAT_800d1dc4;
}
