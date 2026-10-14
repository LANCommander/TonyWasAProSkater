/* Front_DisplayScreen__Fv (0x800161d8) */
#include "names.h"
#ifdef PC_PORT
#include "pc_screen.h"
#endif

extern volatile int GameFade;

void HiRes_BlankScreen(void);
void Mess_LoadFont__FPciii(char *, int, int, int);
void FUN_8008c3d0(int);
void HiRes_ClearOTag(void);
void ResetOT(void);
void SetClippingBox(int, int, int, int);
void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(int, int, int, int);
void Mess_DrawText(int, int, char *, int, int, int, int);
void HiRes_DrawOTag(void);
void Mess_UnloadFont(void);

void Front_DisplayScreen(void) {
	int i;

	HiRes_BlankScreen();
	Mess_LoadFont__FPciii("ingame1.fnt", -1, -1, -1);
#ifdef PC_PORT
	PC_ScreenEnable(1);
#else
	FUN_8008c3d0(1);
#endif

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();

		GameFade = 0;
		SetClippingBox(0, 0, 640, 480);
		Mess_SetTextJustify(0);
		Mess_SetScale(384);
		Mess_SetRGB(150, 100, 25, 0);
		Mess_DrawText(320, 240, "loading...", 1, 0, 256, 0);
		HiRes_DrawOTag();
	} while (i < 2);

	Mess_UnloadFont();
}
