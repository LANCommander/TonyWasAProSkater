/* FrontLoader_UnloadAll__Fv (0x8005ca10) */
#include "names.h"

extern char DAT_800bec34[];

extern int DAT_800d1ca8;

void FUN_80065d80(char *p);

void FrontLoader_UnloadAll(void) {
	if (DAT_800d1ca8)
		FUN_80065d80(DAT_800bec34);

	DAT_800d1ca8 = 0;
}
