/* GotoStartOfTextureList__Fv (0x8006f094) */
#include "names.h"

struct Texture;

extern int HashIndex;
extern struct Texture *pCurrentTex;

extern struct Texture *TextureChecksumHashTable[];

void GotoStartOfTextureList(void) {
	HashIndex = 0;
	pCurrentTex = TextureChecksumHashTable[0];
}
