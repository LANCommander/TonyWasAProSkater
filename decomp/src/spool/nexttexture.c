/* NextTexture__Fv (0x8006f0ac) */
#include "names.h"

typedef struct STexture {
	char pad[0x20];
	struct STexture *next;
} STexture;

extern STexture *TextureChecksumHashTable[];

int HashIndex;
STexture *pCurrentTex;

STexture *NextTexture(void) {
	STexture *t;

	if (HashIndex >= 512)
		return 0;

	t = pCurrentTex;

	if (t) {
	again:
		pCurrentTex = t->next;
		return t;
	}

	do {
		pCurrentTex = t = TextureChecksumHashTable[++HashIndex];

		if (HashIndex >= 512)
			return 0;
	} while (!t);

	goto again;
}
