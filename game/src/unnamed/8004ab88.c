/* RemovePaletteEntry__FP9tag_S_Pal (0x8004ab88) */
#include "names.h"

/* a palette entry (see NewPaletteEntry) */
typedef struct N {
	int a[4];
	struct N *prev;
	struct N *next;
} N;

N *DAT_800d1a80;

void Mem_Delete(void *p);

void RemovePaletteEntry(N *n) {
	N *p = n->prev;
	N *q;

	if (p)
		p->next = n->next;

	q = n->next;

	if (q)
		q->prev = n->prev;

	if (n == DAT_800d1a80)
		DAT_800d1a80 = n->prev;

	Mem_Delete(n);
}
