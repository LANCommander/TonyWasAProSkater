/* FUN_8005d370 (0x8005d370) */
#include "names.h"

unsigned DAT_800d1cf8;

void LoadingScreen_WaitLevel(void);
void LoadingScreen_Wait(void);
void FUN_8005da40(void);

void FUN_8005d370(void) {
	switch (DAT_800d1cf8) {
		case 0:
			LoadingScreen_WaitLevel();
			break;
		case 1:
		case 3:
		case 4:
		case 5:
			LoadingScreen_Wait();
			break;
		case 2:
			FUN_8005da40();
			break;
	}
}
