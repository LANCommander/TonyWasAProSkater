/* draw__6Spriteiiiiii (0x80063be4) */
/* C++ (SHELLUTILS.CPP). Draws a packed sprite's slices (tinted or flat, opaque) or, unpacked, via
   rotateDraw at (x, y). */
#include "game/shellutils.h"

void Sprite::draw(int x, int y, int otFront, int r, int g, int b) {
	if (m_packed)
#ifdef PC_PORT
		m_image->drawScaled(x, y, m_w, m_h, otFront, r, g, b, 0, 0);
#else
		m_image->draw(x, y, otFront, r, g, b, 0, 0);
#endif
	else
		m_image->rotateDraw(x, y);
}
