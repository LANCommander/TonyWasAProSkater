/* FrontLoader_LoadSet__Fi (0x8005c97c) */
#include "names.h"
#ifdef PC_PORT
#include "pc_screen.h"
#endif

int DAT_800d1cac;
int DAT_800d1ca8;

extern int DAT_800d1dc0;
extern char DAT_800bec34[];

void FrontLoader_SetFiles(int a);
void HiRes_BlankScreen(void);
void ClearImage(short *r, int a, int b, int c);
void FUN_80065d14(char *p);

void FrontLoader_LoadSet(int a) {
	short r[4];

	if (DAT_800d1cac != a)
		FrontLoader_SetFiles(a);

	if (DAT_800d1ca8 != 2) {
		if (DAT_800d1dc0) {
			HiRes_BlankScreen();
		} else {
			r[0] = 0;
			r[1] = 0;
			r[2] = 512;
			r[3] = 240;
#ifdef PC_PORT
			PC_ScreenFill(r[0], r[1], r[2], r[3], 0);
#else
			ClearImage(r, 0, 0, 0);
#endif
		}
	}

	FUN_80065d14(DAT_800bec34);

	DAT_800d1ca8 = 2;
}
