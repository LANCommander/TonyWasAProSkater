/* Pack_Unpack__Fss (0x8004a2d4) */
#include "names.h"
typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

TSPack *Pack_pFull;

void Pack_Remove(TSPack **, TSPack *);
TSPack *Pack_NewFree(int, int, int, int);
int Pack_Merge(TSPack *);

void Pack_Unpack(short x, short y) {
	TSPack *p;

	for (p = Pack_pFull; p; p = p->next) {
		if (p->x == x && p->y == y) {
			int w = p->w, h = p->h;

			Pack_Remove(&Pack_pFull, p);
			Pack_Merge(Pack_NewFree(x, y, w, h));
			return;
		}
	}
}
