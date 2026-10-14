/* __6Spriteiiiiiii (0x80063928) */
/* C++ (SHELLUTILS.CPP). A 16-bit sprite grabbed from the VRAM rectangle (x, y, w, h), sliced to at
   most maxw x maxh (0 = whole), black made opaque, optionally packed into VRAM at once. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#include "pc_screen.h"

void *Mem_NewX(unsigned int, int, int);

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
	PC_ScreenRead(rect.x, rect.y, rect.w, rect.h, pData);
	DrawSync(0);

	m_image = new SlicedImage(pData, w, h, maxw, maxh, 16, 0);
	m_image->removeBlack();

	m_packed = packed;

	if (packed)
		m_image->pack();
}
