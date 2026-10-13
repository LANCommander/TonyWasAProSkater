/* Pack_Merge__FP6TSPack (0x8004a590) */
#include "names.h"

typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

TSPack *Pack_pFree;

TSPack *Pack_MergeAdjacent(TSPack *, TSPack *);

int Pack_Merge(TSPack *p) {
	int merged = 0;
	int again;
	TSPack *q;

	do {
		again = 0;

		for (q = Pack_pFree; q; q = q->next) {
			unsigned short py = p->y, qy = q->y;

			if (py == qy && p->h == q->h) {
				if (q->x + q->w == p->x || p->x + p->w == q->x)
					goto merge;
			} else if (p->x == q->x && p->w == q->w && (qy & 0x100) == (py & 0x100)) {
				if (q->y + q->h == p->y || p->y + p->h == q->y)
					goto merge;
			}

			continue;
		merge:
			again = 1;
			p = Pack_MergeAdjacent(q, p);
			merged = 1;
			break;
		}
	} while (again);

	return merged;
}
