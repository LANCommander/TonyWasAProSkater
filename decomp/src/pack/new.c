/* Pack_New__FPP6TSPack (0x8004961c) */
#include "names.h"

typedef struct TSPack {
	unsigned short x, y, w, h;
	struct TSPack *next;
} TSPack;

TSPack *Pack_GetSlot(void);

TSPack *Pack_New(TSPack **list) {
	TSPack *p = Pack_GetSlot();

	p->next = *list;
	*list = p;
	return p;
}
