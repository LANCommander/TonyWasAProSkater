/* FrontLoader_Continue__Fv (0x8005c934) */
#include "names.h"

extern char DAT_800bec34[];

int DAT_800d1ca8;

void FUN_80065ca4(char *p);
int FUN_80065fd4(char *p);

void FrontLoader_Continue(void) {
	DAT_800d1ca8 = 1;
	FUN_80065ca4(DAT_800bec34);

	if (FUN_80065fd4(DAT_800bec34))
		DAT_800d1ca8 = 2;
}
