/* pack__6Sprite (0x80063cb4) */
/* C++ (SHELLUTILS.CPP). Moves the image into VRAM (once). */
#include "game/shellutils.h"

void Sprite::pack() {
	if (!m_packed) {
		m_packed = 1;
		m_image->pack();
	}
}
