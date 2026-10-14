/* Mess_Message__FPCcP12SMessageProgbss (0x8001d644) */
#include "names.h"

typedef struct {
	const char *text;
	short a, b;
	short x, y;
	int flag;
	void *prog;
} Message;

extern Message *CreateMessage(void);

Message *Mess_Message(const char *text, void *prog, int flag, short x, short y) {
	Message *m = CreateMessage();

	m->text = text;
	m->prog = prog;
	m->a = 0;
	m->x = x;
	m->y = y;
	m->flag = flag;
	m->b = 0;
	return m;
}
