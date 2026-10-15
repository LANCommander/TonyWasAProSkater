/* NextTexture__Fv (0x8006f0ac) */
/* C++. Walks the texture checksum hash table, one texture per call. */
#include "game/m3d.h"

extern Texture *TextureChecksumHashTable[];

extern int HashIndex;
extern Texture *pCurrentTex;

Texture *NextTexture(void) {
	Texture *t;

	if (HashIndex >= 512)
		return 0;

	t = pCurrentTex;

	if (t) {
	again:
		pCurrentTex = t->pNext;
		return t;
	}

	do {
		pCurrentTex = t = TextureChecksumHashTable[++HashIndex];

		if (HashIndex >= 512)
			return 0;
	} while (!t);

	goto again;
}
