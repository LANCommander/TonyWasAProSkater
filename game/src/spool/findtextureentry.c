/* Spool_FindTextureEntry__FUi (0x8006f048) */
#include "names.h"

typedef struct STexture {
	char pad[0x14];
	unsigned id;
	char pad2[8];
	struct STexture *next;
} STexture;

extern STexture *TextureChecksumHashTable[];

STexture *Spool_FindTextureEntry(unsigned id) {
	STexture *t = TextureChecksumHashTable[id & 511];

	while (t && t->id != id)
		t = t->next;

	return t;
}
