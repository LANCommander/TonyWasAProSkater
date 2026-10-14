/* UberFrig__FP6CBruceG7CVectorPc (0x800301b0) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	V s, e;
	char pad[0x68 - 24];
	int hit;
	V h;
	char pad2[0x80 - 0x78];
	int face;
	char pad3[0xa8 - 0x84];
} L;

typedef struct {
	char p0[4];
	V pos;
	char p1[0x6c - 0x10];
	V vel;
	char p2[0xf0 - 0x78];
	V old;
	char p3[0x1bc - 0xfc];
	V v1bc;
	char p4[0x6dc - 0x1c8];
	int f6dc;
} B;

void CBruce_CalculateDynamicLighting(B *, int, int);
int M3dZone_LineToItemNotSCached(L *, int);
void M3dColij_InitLineInfo(L *);

void UberFrig(B *p, V a, char *msg) {
	L li;

	if (p->f6dc == 0) {
		li.s.x = a.x;
		li.s.y = a.y;
		li.s.z = a.z;
		li.e = li.s;
		li.e.y = li.e.y + 0x1f40000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.hit) {
			CBruce_CalculateDynamicLighting(p, li.face, li.hit);
			return;
		}

		li.s.x = a.x;
		li.s.y = a.y;
		li.s.z = a.z;
		li.s.y = a.y + -0x12c000;
		li.e = li.s;
		li.e.y = li.e.y + 0x190000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.hit) {
			p->pos.x = li.h.x;
			p->pos.y = li.h.y;
			p->pos.z = li.h.z;
			p->pos.y = li.h.y + -0x1e000;
			p->old = p->pos;
			return;
		}

		p->pos = p->v1bc;
		p->old = p->pos;
		p->vel.x = -p->vel.z / 2;
		p->vel.z = p->vel.x / 2;
	}
}
