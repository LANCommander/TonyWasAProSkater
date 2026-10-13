/* __6Spriteiiiiiii (0x80063928) */
/* C++ (SHELLUTILS.CPP). A 16-bit sprite grabbed from the VRAM rectangle (x, y, w, h), sliced to at
   most maxw x maxh (0 = whole), black made opaque, optionally packed into VRAM at once. */
#include "game/shellutils.h"

void *Mem_NewX(unsigned int, int, int);

extern "C" {
	int FUN_8008c468(int);            /* DrawSync */
	int FUN_8008c774(RECT *, void *); /* StoreImage */
}

Sprite::Sprite(int x, int y, int w, int h, int packed, int maxw, int maxh) {
	RECT rect;
	char *pData;

	m_bpp = 16;
	m_w = w;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	m_h = h;
	rect.h = h;

	if (maxw >= w)
		maxw = 0;

	if (maxh >= h)
		maxh = 0;

	pData = (char *)Mem_NewX(w * h * 2, 1, 1);
	FUN_8008c774(&rect, pData);
	FUN_8008c468(0);

	m_image = new SlicedImage(pData, w, h, maxw, maxh, 16, 0);
	m_image->removeBlack();

	m_packed = packed;

	if (packed)
		m_image->pack();
}
