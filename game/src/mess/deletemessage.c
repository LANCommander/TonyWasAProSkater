/* Mess_DeleteMessage__FP7Message (0x8001d590) */
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

Message *pMessages;

extern void Mem_Delete(void *);

void Mess_DeleteMessage(Message *m) {
	if (m->next)
		m->next->prev = m->prev;

	if (m->prev)
		m->prev->next = m->next;

	if (m == pMessages)
		pMessages = m->next;

	Mem_Delete(m);
}
