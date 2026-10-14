/* __11EraseSpriteiiii (0x80063d48) */
/* C++ (SHELLUTILS.CPP). Saves the VRAM rectangle (x, y, w, h) to main RAM so it can be restored. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#ifdef PC_PORT
#include "pc_screen.h"
#endif

void *Mem_NewX(unsigned int, int, int);

EraseSprite::EraseSprite(int x, int y, int w, int h) {
	mR.x = x;
	mR.y = y;
	mR.w = w;
	mR.h = h;
	mData = (ulong *)Mem_NewX(w * h * 2, 1, 1);
#ifdef PC_PORT
	PC_ScreenRead(mR.x, mR.y, mR.w, mR.h, mData);
#else
	StoreImage(&mR, mData);
#endif
}
