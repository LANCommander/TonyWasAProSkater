/* CreateMessage__Fv (0x8001d550) */
#include "names.h"

typedef struct Message {
	char pad[0x14];
	struct Message *next;
	struct Message *prev;
} Message;

Message *pMessages;

extern Message *Mem_NewX(int, int, int);

void CreateMessage(void) {
	Message *m = Mem_NewX(0x1c, 0, 1);
	Message *h = pMessages;

	m->prev = 0;
	pMessages = m;
	m->next = h;

	if (h)
		h->prev = m;
}
