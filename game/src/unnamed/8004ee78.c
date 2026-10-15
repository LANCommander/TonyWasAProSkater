/* TrickOb_DeleteAll__Fv (0x8004ee78) */
#include "names.h"

struct VT {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
	char pad[12];
	struct O *next;
	char pad2[16];
	struct O *next2;
};

struct O *TrickObList;

void TrickOb_DeleteAll(void) {
	struct O *p = TrickObList;
	struct O *n;

	while (p) {
		n = p->next;

		if (p)
			p->vt->fn((char *)p + p->vt->off, 3);

		p = n;
	}
}
