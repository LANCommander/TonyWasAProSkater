/* Front_FreeMemCardMenus__Fv (0x80011150) */
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
};

extern struct O *Tri1;
extern struct O *Tri2;
extern struct O *pVertMenu;

void Front_FreeMemCardMenus(void) {
	struct O *o = Tri1;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	o = Tri2;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	o = pVertMenu;
	Tri2 = 0;
	Tri1 = 0;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	pVertMenu = 0;
}
