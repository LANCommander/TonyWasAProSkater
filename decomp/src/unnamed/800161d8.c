/* FUN_800161d8 (0x800161d8) */
#include "names.h"
extern char s_ingame1_fnt[];
extern char s_loading[];
extern volatile int GameFade;

void FUN_8006af64(void);
void Mess_LoadFont__FPciii(char *, int, int, int);
void FUN_8008c3d0(int);
void FUN_8006b240(void);
void FUN_80061ea0(void);
void SetClippingBox(int, int, int, int);
void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(int, int, int, int);
void Mess_DrawText(int, int, char *, int, int, int, int);
void FUN_8006b26c(void);
void Mess_UnloadFont(void);

void FUN_800161d8(void) {
	int i;

	FUN_8006af64();
	Mess_LoadFont__FPciii(s_ingame1_fnt, -1, -1, -1);
	FUN_8008c3d0(1);

	i = 0;

	do {
		i++;
		FUN_8006b240();
		FUN_80061ea0();

		GameFade = 0;
		SetClippingBox(0, 0, 640, 480);
		Mess_SetTextJustify(0);
		Mess_SetScale(384);
		Mess_SetRGB(150, 100, 25, 0);
		Mess_DrawText(320, 240, s_loading, 1, 0, 256, 0);
		FUN_8006b26c();
	} while (i < 2);

	Mess_UnloadFont();
}
