/* _._11EraseSprite (0x80063db4) */
/* C++ (SHELLUTILS.CPP). Frees the saved pixels. */
#include "game/shellutils.h"

void Mem_Delete(void *);

EraseSprite::~EraseSprite() {
	Mem_Delete(mData);
}
