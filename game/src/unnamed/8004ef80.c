/* FUN_8004ef80 (0x8004ef80) */
struct L {
	char *vt;
	int a, b, c;
	struct L *next;
};

struct L *TrickObList;

extern char __vt_8CTrickOb[];
extern char DAT_800a1330[];

void __dl__6CClassPv(void *p);

void FUN_8004ef80(struct L *p, int f) {
	struct L *prev = 0;
	struct L *c = TrickObList;

	p->vt = __vt_8CTrickOb;

	while (c != p) {
		prev = c;
		c = c->next;
	}

	if (prev)
		prev->next = p->next;
	else
		TrickObList = p->next;

	p->vt = DAT_800a1330;

	if (f & 1)
		__dl__6CClassPv(p);
}
