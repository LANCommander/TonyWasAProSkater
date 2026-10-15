/* FUN_80063a5c (0x80063a5c) */
/* C++ (SHELLUTILS.CPP): a Sprite constructor, kept as the egcs-ABI function retail has: loads an
   8-bit BMP into a SlicedImage and loads that to VRAM at (a3, x). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"

extern "C" char __vt_6Sprite[];

int Load8BitBMP2(char *name, char **a, int *b, int *c, unsigned short *d);
extern "C" void FUN_800627ec(SlicedImage *p, int a, int b); /* SlicedImage::packAt */

extern "C" Sprite *FUN_80063a5c(Sprite *p, int f16, char *name, int a3, int x, int mw, int mh) {
	unsigned short pal;
	char *data;
	int w;
	int h;
	SlicedImage *v;

	p->__vptr = __vt_6Sprite;
	Load8BitBMP2(name, &data, &w, &h, &pal);

	p->m_bpp = f16;
	p->m_w = w;
	p->m_h = h;

	if (!(mw * 2 < w))
		mw = 0;

	if (!(mh < h))
		mh = 0;

	v = new SlicedImage(data, w, h, mw, mh, p->m_bpp, pal);

	p->m_image = v;
	data = 0;
	p->m_packed = 1;
	FUN_800627ec(v, a3, x);
	return p;
}
