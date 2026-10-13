/* Mess_DeleteMessage__FP7Message (0x8001d590) */
#include "names.h"

typedef struct Message {
	char pad[0x14];
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
