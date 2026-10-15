/* FUN_8004f684 (0x8004f684) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct P {
	void *vt;
	int f4;
	V mn;
	V mx;
	struct P *prev;
	struct P *next;
	int f28;
} P;

typedef struct {
	char p[6];
	short s6;
} Q;

extern char __vt_5CRail[];

extern P *RailList;

unsigned short *Trig_GetLinksPointer(int);
Q *Trig_GetPosition(V *, int);
int Rail_NextNode(unsigned short *);

P *FUN_8004f684(P *p, int idx) {
	V a, b;
	int pa, pb;
	short s;
	unsigned short *l;

	p->vt = __vt_5CRail;

	if (RailList)
		RailList->prev = p;

	p->next = RailList;
	p->prev = 0;
	RailList = p;
	Trig_GetLinksPointer(idx);

	p->f4 = idx;
	s = Trig_GetPosition(&a, idx)->s6;

	if (s == -1)
		p->f28 = 0;
	else
		p->f28 = s;

	idx = p->f4;
	p->mn.z = 0x7fffffff;
	p->mn.y = 0x7fffffff;
	p->mn.x = 0x7fffffff;
	p->mx.z = 0x80000000;
	p->mx.y = 0x80000000;
	p->mx.x = 0x80000000;

	while (1) {
		Trig_GetPosition(&b, idx);

		if (b.x < p->mn.x)
			p->mn.x = b.x;

		if (b.y < p->mn.y)
			p->mn.y = b.y;

		if (b.z < p->mn.z)
			p->mn.z = b.z;

		if (b.x > p->mx.x)
			p->mx.x = b.x;

		if (b.y > p->mx.y)
			p->mx.y = b.y;

		if (b.z > p->mx.z)
			p->mx.z = b.z;

		l = Trig_GetLinksPointer(idx);

		if (*l == 0)
			break;

		idx = Rail_NextNode(l);
	}

	return p;
}
