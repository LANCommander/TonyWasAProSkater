/* Flash_Reset__Fv (0x8001f310) */
#include "names.h"

int FlashCountdown;
int FadeCountdown;
int Fading;
char CurrentImportance;

void Flash_Reset(void) {
	FlashCountdown = 0;
	FadeCountdown = 0;
	Fading = 0;
	CurrentImportance = 0;
}
