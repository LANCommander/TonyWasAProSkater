/* restore__11EraseSpriteii (0x80063e10) */
/* C++ (SHELLUTILS.CPP). Puts the saved pixels back where they came from (x, y unused). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"

void EraseSprite::restore(int x, int y) {
	LoadImage(&mR, mData);
}
