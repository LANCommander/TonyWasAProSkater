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
		/* (retail read the entry past the table's end before this test; the value was never used:
		   GotoStartOfTextureList sets both again) */
		if (++HashIndex >= 512) {
			pCurrentTex = 0;
			return 0;
		}

		pCurrentTex = t = TextureChecksumHashTable[HashIndex];
	} while (!t);

	goto again;
}
