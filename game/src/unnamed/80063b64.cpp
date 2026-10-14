/* _._6Sprite (0x80063b64) */
/* C++ (SHELLUTILS.CPP). Deletes the sprite's sliced image. */
#include "game/shellutils.h"

Sprite::~Sprite() {
	delete m_image;
}
