/* Flash_Fade__Fii (0x8001f3b8) */
#include "names.h"

extern int Fading, FadeCountdown, FlashSort, CurrentB, CurrentG, CurrentR, dR;
extern unsigned char CurrentImportance;

void Flash_Fade(int a, int b) {
	Fading = 1;
	FadeCountdown = a;
	CurrentImportance = 255;
	CurrentB = 0;
	CurrentG = 0;
	CurrentR = 0;
	FlashSort = b;
	dR = 0xff0000 / a;
}
