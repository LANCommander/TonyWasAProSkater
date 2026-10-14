/* Pack_Remove__FPP6TSPackP6TSPack (0x80049710) */
#include "names.h"

typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

void Pack_KillSlot(TSPack *);

void Pack_Remove(TSPack **list, TSPack *p) {
	TSPack *q;

	if (*list == p) {
		*list = p->next;
	} else {
		q = *list;

		while (q->next != p)
			q = q->next;

		q->next = p->next;
	}

	Pack_KillSlot(p);
}
