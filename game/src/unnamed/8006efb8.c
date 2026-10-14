/* RemoveTextureEntry__FP7Texture (0x8006efb8) */
#include "names.h"

struct N {
	char pad[16];
	unsigned f;
	unsigned id;
	char pad2[8];
	struct N *a;
	struct N *b;
};

extern struct N *TextureChecksumHashTable[];

void Mem_Delete(struct N *p);

void RemoveTextureEntry(struct N *p) {
	struct N **s;

	if (p->a)
		p->a->b = p->b;

	if (p->b)
		p->b->a = p->a;

	s = &TextureChecksumHashTable[p->id & 511];

	if (p == *s)
		*s = p->a;

	Mem_Delete(p);
}
