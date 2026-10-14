/* NewTextureEntry__FUi (0x8006ef30) */
/* C++ (SPOOL.CPP). Demo twin; locals from its MAIN.SYM. Allocates a texture entry for a checksum
   and links it at the head of its hash bucket (low 9 bits of the checksum). */
#include "game/m3d.h"
#include "game/mem.h"

extern Texture *TextureChecksumHashTable[];

Texture *NewTextureEntry(uint checksum) {
	Texture *pNew = Mem_NewX(40, 0, 1);
	uint Lower = checksum & 511;

	pNew->pNext = TextureChecksumHashTable[Lower];
	pNew->pPrevious = 0;
	TextureChecksumHashTable[Lower] = pNew;

	if (pNew->pNext)
		pNew->pNext->pPrevious = pNew;

	pNew->Checksum = checksum;
	pNew->InVRAM = 0;
	pNew->Usage = 0;
	return pNew;
}
