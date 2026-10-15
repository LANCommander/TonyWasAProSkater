/* Spool_FindTextureEntry__FUi (0x8006f048) */
#include "game/m3d.h"

extern Texture *TextureChecksumHashTable[];

Texture *Spool_FindTextureEntry(uint id) {
	Texture *t = TextureChecksumHashTable[id & 511];

	while (t && t->Checksum != id)
		t = t->pNext;

	return t;
}
