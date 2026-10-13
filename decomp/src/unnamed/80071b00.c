/* DeleteList__FP5CBody (0x80071b00) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	char pad0[28];
	struct O *next;
	char pad[40];
	struct VT *vt;
	char pad3[6];
	unsigned short fl;
};

void DeleteList(struct O *p) {
	struct O *n;

	if (!p)
		return;

	n = p->next;

	for (;;) {
		if (!(p->fl & 32)) {
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
