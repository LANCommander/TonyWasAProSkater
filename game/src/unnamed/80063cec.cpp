/* unpack__6Sprite (0x80063cec) */
/* C++ (SHELLUTILS.CPP). Releases the image's VRAM if it was packed. */
#include "game/shellutils.h"

void Sprite::unpack() {
	if (m_packed) {
		m_packed = 0;
		m_image->unpack();
	}
}
