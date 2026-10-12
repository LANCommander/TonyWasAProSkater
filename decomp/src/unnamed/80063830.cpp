/* __6SpritePciii (0x80063830) */
/* C++ (SHELLUTILS.CPP). An 8-bit sprite loaded from a BMP, sliced to at most maxw/2 x maxh (0 =
   whole image when the image is no bigger), optionally packed into VRAM at once. */
#include "game/shellutils.h"

int Load8BitBMP(char *, char **, int *, int *, ushort *);

Sprite::Sprite(char *pName, int packed, int maxw, int maxh) {
	ushort clut;
	char *pData;
	int w;
	int h;

	Load8BitBMP(pName, &pData, &w, &h, &clut);

	m_bpp = 8;
	m_w = w;
	m_h = h;

	if (!(maxw * 2 < w))
		maxw = 0;

	if (!(maxh < h))
		maxh = 0;

	m_image = new SlicedImage(pData, w, h, maxw, maxh, m_bpp, clut);
	m_packed = packed;

	if (packed)
		m_image->pack();
}
