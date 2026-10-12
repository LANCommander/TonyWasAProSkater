/* __11EraseSpriteiiii (0x80063d48) */
/* C++ (SHELLUTILS.CPP). Saves the VRAM rectangle (x, y, w, h) to main RAM so it can be restored. */
#include "game/shellutils.h"

void *Mem_NewX(unsigned int, int, int);

extern "C" int FUN_8008c774(RECT *, void *); /* StoreImage */

EraseSprite::EraseSprite(int x, int y, int w, int h) {
	mR.x = x;
	mR.y = y;
	mR.w = w;
	mR.h = h;
	mData = (ulong *)Mem_NewX(w * h * 2, 1, 1);
	FUN_8008c774(&mR, mData);
}
