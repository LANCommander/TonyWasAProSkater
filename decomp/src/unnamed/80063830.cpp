/* __6SpritePciii (0x80063830) */
/* C++ (SHELLUTILS.CPP). An 8-bit sprite loaded from a BMP, sliced to at most maxw/2 x maxh (0 =
   whole image when the image is no bigger), optionally packed into VRAM at once. */
#include "game/shellutils.h"

int Load8BitBMP(char *, char **, int *, int *, ushort *);

#ifdef PC_PORT
#include "pc_assets.h"
#endif

Sprite::Sprite(char *pName, int packed, int maxw, int maxh) {
	ushort clut;
	char *pData;
	int w;
	int h;

	Load8BitBMP(pName, &pData, &w, &h, &clut);

	m_bpp = 8;
	m_w = w;
	m_h = h;

#ifdef PC_PORT
	/* an image the Dreamcast redrew larger (pc_assets.h): laid out at the PS1's size, drawn
	   scaled to it with all its pixels (draw, drawFaded) */
	{
		int lw, lh;

		if (PC_ImageLayoutSize(pName, w, h, &lw, &lh)) {
			m_w = lw;
			m_h = lh;
		}
	}
#endif

	if (!(maxw * 2 < w))
		maxw = 0;

	if (!(maxh < h))
		maxh = 0;

	m_image = new SlicedImage(pData, w, h, maxw, maxh, m_bpp, clut);
	m_packed = packed;

	if (packed)
		m_image->pack();
}
