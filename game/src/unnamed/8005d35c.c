/* LoadingScreen_SetCurrentLevel__Fiii (0x8005d35c) */
#include "names.h"

extern int DAT_800d1cf4;
extern int DAT_800d1cf8;
extern int DAT_800d234c;

void LoadingScreen_SetCurrentLevel(int a, int b, int c) {
	DAT_800d1cf4 = a;
	DAT_800d1cf8 = b;
	DAT_800d234c = c;
}
