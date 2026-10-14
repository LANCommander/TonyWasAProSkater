/* restore__11EraseSpriteii (0x80063e10) */
/* C++ (SHELLUTILS.CPP). Puts the saved pixels back where they came from (x, y unused). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#ifdef PC_PORT
#include "pc_screen.h"
#endif

void EraseSprite::restore(int x, int y) {
#ifdef PC_PORT
	PC_ScreenBlit(mR.x, mR.y, mR.w, mR.h, mData);
#else
	LoadImage(&mR, mData);
#endif
}
