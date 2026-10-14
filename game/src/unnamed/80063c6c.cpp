/* drawFaded__6Spriteiiiiii (0x80063c6c) */
/* C++ (SHELLUTILS.CPP). Draws the sprite semi-transparent. */
#include "game/shellutils.h"

void Sprite::drawFaded(int x, int y, int otFront, int r, int g, int b) {
	m_image->drawScaled(x, y, m_w, m_h, otFront, r, g, b, 1, 0);
}
