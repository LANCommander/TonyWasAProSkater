/* FUN_80071a14 (0x80071a14) */
#include "names.h"

struct L {
	char *vt;
	char pad[40];
	int *m1;
	int *m2;
	char pad2[16];
	struct L *prev;
	struct L *next;
};

extern char DAT_800a2450[];
extern char DAT_800a2460[];

extern struct L *DAT_800d1f78;

void Mem_Delete(int *p);
void __dl__6CClassPv(void *p);

void FUN_80071a14(struct L *p, int f) {
	p->vt = DAT_800a2450;

	if (p->prev)
		p->prev->next = p->next;

	if (p->next)
		p->next->prev = p->prev;

	if (DAT_800d1f78 == p)
		DAT_800d1f78 = p->prev;

	Mem_Delete(p->m1);
	Mem_Delete(p->m2);

	p->vt = DAT_800a2460;

	if (f & 1)
		__dl__6CClassPv(p);
}
