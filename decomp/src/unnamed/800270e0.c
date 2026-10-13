/* FUN_800270e0 (0x800270e0) */
#include "names.h"

extern char __vt_9CPolyLine[];
extern int PolyLineList;

struct V {
	int x, y, z;
};

struct E {
	struct V v;
	int w;
};

extern struct V ZeroVector;

struct D {
	char pad[68];
	char *vt;
	int q;
	int n;
	struct E *e;
};

int CBit_ctor(struct D *p);

struct E *Mem_NewX(int a, int b, int c);

void CBit_AttachTo(struct D *p, int *q);

struct D *FUN_800270e0(struct D *p, int n) {
	int i;

	CBit_ctor(p);

	p->vt = __vt_9CPolyLine;
	p->e = Mem_NewX(n << 4, 0, 1);

	for (i = 0; i < n; i++) {
		p->e[i].w = 0x40ffffff;
		p->e[i].v = ZeroVector;
	}

	p->n = n;
	CBit_AttachTo(p, &PolyLineList);
	return p;
}
