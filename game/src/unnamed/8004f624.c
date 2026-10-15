/* Rail_DeleteAll__Fv (0x8004f624) */
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

struct O *RailList;

void Rail_DeleteAll(void) {
	struct O *p = RailList;
	struct O *n;

	while (p) {
		n = p->next2;

		if (p)
			p->vt->fn((char *)p + p->vt->off, 3);

		p = n;
	}
}
