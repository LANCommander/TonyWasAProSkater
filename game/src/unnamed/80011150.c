/* Front_FreeMemCardMenus__Fv (0x80011150) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

struct O *Tri1;
struct O *Tri2;
struct O *pVertMenu;

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
