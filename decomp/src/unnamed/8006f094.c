/* GotoStartOfTextureList__Fv (0x8006f094) */
#include "names.h"

int HashIndex;
int pCurrentTex;

extern int TextureChecksumHashTable[];

void GotoStartOfTextureList(void) {
	HashIndex = 0;
	pCurrentTex = TextureChecksumHashTable[0];
}
