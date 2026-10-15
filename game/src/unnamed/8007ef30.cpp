/* FUN_8007ef30 (0x8007ef30) */
#include "game/colij.h"

extern int ItemCacheIndex;
extern int DAT_800d20e4;
extern int DAT_800d20e8;

extern SColCache ItemCache[10];

extern "C" void FUN_8007ef30() {
	int i;

	ItemCacheIndex = 0;
	DAT_800d20e4 = 0;
	DAT_800d20e8 = -1;

	for (i = 0; i < 10; i++)
		ItemCache[i].pItem = 0;
}
