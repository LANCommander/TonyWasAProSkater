/* CreateMessage__Fv (0x8001d550) */
#include "names.h"

/* the whole layout (Mess_Message, Mess_Update): in the 64-bit port text/prog are 8 bytes */
typedef struct Message {
	const char *text;
	unsigned short cnt, cnt2;
	short x, y;
	int flag;
	void *prog;
	struct Message *next;
	struct Message *prev;
} Message;

extern Message *pMessages;

extern Message *Mem_NewX(int, int, int);

/* the new message, first in the list (Mess_Message uses it) */
Message *CreateMessage(void) {
	Message *m = Mem_NewX(sizeof(Message), 0, 1);
	Message *h = pMessages;

	m->prev = 0;
	pMessages = m;
	m->next = h;

	if (h)
		h->prev = m;

	return m;
}
