/* restore__11EraseSpriteii (0x80063e10) */
/* C++ (SHELLUTILS.CPP). Puts the saved pixels back where they came from (x, y unused). */
#include "game/shellutils.h"

extern "C" int FUN_8008c714(RECT *, void *); /* LoadImage */

void EraseSprite::restore(int x, int y) {
	FUN_8008c714(&mR, mData);
}
