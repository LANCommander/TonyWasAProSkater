/* Mess_DeleteAll__Fv (0x8001d5fc) */
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

void Mess_DeleteAll(void) {
	Message *m = pMessages;
	Message *n;

	if (m) {
		n = m->next;
	loop:
		Mem_Delete(m);

		if (n) {
			m = n;
			n = n->next;
			goto loop;
		}
	}

	pMessages = 0;
}
