/* DeleteBitList__FP4CBit (0x80027ab0) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	char pad0[4];
	struct O *next;
	char pad[58];
	unsigned char c;
	char p2;
	struct VT *vt;
};

void DeleteBitList(struct O *p) {
	struct O *n;

	if (!p)
		return;

	n = p->next;

	for (;;) {
		if (p->c == 0) {
			if (p)
				p->vt->fn((char *)p + p->vt->off, 3);
		}

		p = n;

		if (n)
			n = n->next;
		else
			break;
	}
}
