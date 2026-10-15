/* NewPaletteEntry__FUi (0x8004ab2c) */
#include "names.h"

struct N {
	int a[1];
	short s;
	char c;
	char c2;
	int x;
	int y;
	struct N *prev;
	struct N *next;
};

struct N *DAT_800d1a80;
struct N *Mem_NewX(int a, int b, int c);

struct N *NewPaletteEntry(int p) {
	struct N *n = Mem_NewX(sizeof(struct N), 0, 1);
	struct N *h = DAT_800d1a80;

	n->next = 0;
	DAT_800d1a80 = n;
	n->prev = h;

	if (h)
		h->next = n;

	n->x = p;
	n->c = 0;
	n->s = 0;
	return n;
}
