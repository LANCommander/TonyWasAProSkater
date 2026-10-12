/* removeFromMemory__6Sprite (0x80063d24) */
/* C++ (SHELLUTILS.CPP). Frees the image's pixel copy in main RAM. */
#include "game/shellutils.h"

void Sprite::removeFromMemory() {
	m_image->removeFromMemory();
}
