/* RemoveTextureEntry__FP7Texture (0x8006efb8) */
#include "game/m3d.h"
#include "game/mem.h"

extern Texture *TextureChecksumHashTable[512];

void RemoveTextureEntry(Texture *p) {
	Texture **s;

	if (p->pNext)
		p->pNext->pPrevious = p->pPrevious;

	if (p->pPrevious)
		p->pPrevious->pNext = p->pNext;

	s = &TextureChecksumHashTable[p->Checksum & 511];

	if (p == *s)
		*s = p->pNext;

	Mem_Delete(p);
}
