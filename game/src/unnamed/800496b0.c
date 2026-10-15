/* Pack_NewFull__Fiiii (0x800496b0) */
#include "names.h"

typedef struct {
	short a, b, c, d;
} TSPack;

extern TSPack *Pack_pFull;

TSPack *Pack_New(TSPack **);

TSPack *Pack_NewFull(int a, int b, int c, int d) {
	TSPack *p = Pack_New(&Pack_pFull);

	p->a = a;
	p->b = b;
	p->c = c;
	p->d = d;
	return p;
}
