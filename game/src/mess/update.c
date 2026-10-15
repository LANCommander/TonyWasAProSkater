/* Mess_Update__Fv (0x8001d6bc) */
#include "names.h"

typedef struct Prog {
	unsigned short dur;
	char pad[0x1c];
	signed char cmd;
	char pad2;
} Prog;

typedef struct Message {
	const char *text; /* (a pointer: 8 bytes in the 64-bit port) */
	unsigned short cnt, cnt2;
	short x, y;
	int flag;
	Prog *prog;
	struct Message *next;
	struct Message *prev;
} Message;

Message *pMessages;

extern void Mess_DeleteMessage(Message *);

void Mess_Update(void) {
	Message *m = pMessages;
	Message *n;

	if (!m)
		return;

	n = m->next;

	for (;;) {
		Prog *p = m->prog;

		if (m->cnt >= p->dur) {
			signed char c = p->cmd;

			if (c == 0) {
				Mess_DeleteMessage(m);
			} else if (c != 127) {
				m->prog = p + c;
				m->cnt = 0;
				m->cnt2++;
			}
		} else {
			m->cnt++;
			m->cnt2++;
		}

		m = n;

		if (n)
			n = n->next;
		else
			break;
	}
}
