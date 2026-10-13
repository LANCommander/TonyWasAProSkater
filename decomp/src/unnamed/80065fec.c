/* FUN_80065fec (0x80065fec) */
#include "names.h"

typedef struct {
	void *a;
	int p4[5];
	int f18;
	int sz;
	int f20;
} E;

typedef struct {
	int p0[2];
	int n;
	int pc;
	E e[1];
} S;

extern int Mem_NewTop(int a);
extern int Mem_NewX(int a, int b, int c);
extern int Mem_Delete(void *a);
extern int Mem_Copy(void *a, void *b, int c);

void FUN_80065fec(S *s) {
	int tmp[32];
	int i;

	for (i = 0; i < s->n; i++) {
		if (s->e[i].f20 && !s->e[i].f18) {
			tmp[i] = Mem_NewTop(s->e[i].sz);
			Mem_Copy((void *)tmp[i], s->e[i].a, s->e[i].sz);
			Mem_Delete(s->e[i].a);
		}
	}

	for (i = 0; i < s->n; i++) {
		if (s->e[i].f20 && !s->e[i].f18) {
			s->e[i].a = (void *)Mem_NewX(s->e[i].sz, 1, 1);
			Mem_Copy(s->e[i].a, (void *)tmp[i], s->e[i].sz);
			Mem_Delete((void *)tmp[i]);
		}
	}
}
