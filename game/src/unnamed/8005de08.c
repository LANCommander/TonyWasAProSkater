/* FUN_8005de08 (0x8005de08) */
#include "names.h"

extern int DAT_800bf0f4[];
extern int DAT_800bf100[];
extern char DAT_800bf104[];

int DAT_800d1cc8;
int DAT_800d1ccc;

void PadBuffer_ctor(char *p, int a);

void FUN_8005de08(void) {
	DAT_800d1cc8 = DAT_800bf0f4[0];
	DAT_800d1ccc = DAT_800bf100[0];
	PadBuffer_ctor(DAT_800bf104, -1);
}
