/* Pack_NewFree__Fiiii (0x80049650) */
#include "names.h"

typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

TSPack *Pack_pFree;

TSPack *Pack_New(TSPack **);

TSPack *Pack_NewFree(int x, int y, int w, int h) {
	TSPack *p = Pack_New(&Pack_pFree);

	p->x = x;
	p->y = y;
	p->w = w;
	p->h = h;
	return p;
}
