/* Medal_Create__F10EMedalTypeiiiii (0x80073dc8) */
#include "names.h"

typedef struct {
	short vx, vy, vz, pad;
} SVECTOR;

typedef struct {
	int vx, vy, vz, pad;
} VECTOR;

#ifndef PC_PORT
#define CTC2(n, v) __asm__ volatile("ctc2 %0, $" #n : : "r"(v))
#define MTC2(n, v) __asm__ volatile("mtc2 %0, $" #n : : "r"(v))
#define STLVNL(p)                                                                                  \
	__asm__ volatile("swc2 $25, 0(%0); swc2 $26, 4(%0); swc2 $27, 8(%0)" : : "r"(p) : "memory")
#endif

typedef struct {
	unsigned short flags;
	short s2, s4;
	char _6[0x1e];
	short s36, s38, s40;
	char _2a[300];
	short s342, s344, s346;
} Medal;

extern char *pCurrentCamera;
extern char ZeroVector[];

Medal *__nw__5CItemUi(int);
Medal *CPowerUp_ctor(Medal *, int, int *, char *, int, int);

Medal *Medal_Create(int type, int x, int y, int z, int a, int b) {
	SVECTOR v;
	VECTOR o;
	int pos[3];
	Medal *p;
	short sa;
	short *q;

	v.vx = x;
	v.vy = y;
	v.vz = z;
#ifdef PC_PORT
	{
		/* v through the camera's transform, unshifted */
		GeomAcc r;

		geom_ld_mat(pc_geom.rot, pCurrentCamera + 0x34);
		r = geom_mvmva(pc_geom.rot, 0, v.vx, v.vy, v.vz, 0, 0, 0);
		geom_st_lv(&o, r.mac);
	}
#else
	CTC2(0, *(int *)(pCurrentCamera + 0x34));
	CTC2(1, *(int *)(pCurrentCamera + 0x38));
	CTC2(2, *(int *)(pCurrentCamera + 0x3c));
	CTC2(3, *(int *)(pCurrentCamera + 0x40));
	CTC2(4, *(int *)(pCurrentCamera + 0x44));
	MTC2(0, *(int *)&v.vx);
	MTC2(1, *(int *)&v.vz);
	__asm__ volatile("nop; nop; cop2 0x406012");

	STLVNL(&o);
#endif

	pos[0] = o.vx + (*(int *)(pCurrentCamera + 4) << 12);
	pos[1] = o.vy + (*(int *)(pCurrentCamera + 8) << 12);
	pos[2] = o.vz + (*(int *)(pCurrentCamera + 12) << 12);

	switch (type) {
		case 5:
			p = __nw__5CItemUi(356);

			if (p)
				p = CPowerUp_ctor(p, 1636, pos, ZeroVector, 0, 0xffff);

			break;
		case 6:
			p = __nw__5CItemUi(356);

			if (p)
				p = CPowerUp_ctor(p, 1637, pos, ZeroVector, 0, 0xffff);

			break;
		case 7:
			p = __nw__5CItemUi(356);

			if (p)
				p = CPowerUp_ctor(p, 1638, pos, ZeroVector, 0, 0xffff);

			break;
	}

	q = &p->s36;
	p->s346 = 80;
	sa = a;
	p->flags |= 0x200;
	p->s36 = sa;
	q[1] = sa;
	q[2] = sa;
	p->s344 = b;
	p->s342 = b;
	return p;
}
