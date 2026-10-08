/* restore__11EraseSpriteii (0x80063e10) */
#include "names.h"
void FUN_8008c714(char *p, int a);

void EraseSprite_restore(char *p, int x, int y) {
	FUN_8008c714(p + 4, *(int *)(p + 12));
}
