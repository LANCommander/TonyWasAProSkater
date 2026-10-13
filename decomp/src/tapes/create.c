/* Tape_Create__Fiiiiiiii (0x80073a64) */
#include "names.h"

typedef struct {
	short vx, vy, vz, pad;
} SVECTOR;

typedef struct {
	int x, y, z;
} V;

#define CTC2(n, v) __asm__ volatile("ctc2 %0, $" #n : : "r"(v))
#define MTC2(n, v) __asm__ volatile("mtc2 %0, $" #n : : "r"(v))
#define STLVNL(p)                                                                                  \
	__asm__ volatile("swc2 $25, 0(%0); swc2 $26, 4(%0); swc2 $27, 8(%0)" : : "r"(p) : "memory")

typedef struct {
	unsigned short flags;
	char _2[0x18c];
	short s398, s400, s402;
} Tape;

extern char *pCurrentCamera;
extern char ZeroVector[];

Tape *__nw__5CItemUi(int);
Tape *CTape_ctor(Tape *, int, V *, V *, int, int, char *, int, int);

Tape *Tape_Create(int x, int y, int z, int p, int e, int f, int g, int h) {
	SVECTOR v;
	V o, pos1, pos2;
	Tape *t;

	{
		v.vx = x;
		v.vy = y;
		v.vz = z;
		CTC2(0, *(int *)(pCurrentCamera + 0x34));
		CTC2(1, *(int *)(pCurrentCamera + 0x38));
		CTC2(2, *(int *)(pCurrentCamera + 0x3c));
		CTC2(3, *(int *)(pCurrentCamera + 0x40));
		CTC2(4, *(int *)(pCurrentCamera + 0x44));
		MTC2(0, *(int *)&v.vx);
		MTC2(1, *(int *)&v.vz);
		__asm__ volatile("nop; nop; cop2 0x406012");

		STLVNL(&o);

		pos1.x = o.x + (*(int *)(pCurrentCamera + 4) << 12);
		pos1.y = o.y + (*(int *)(pCurrentCamera + 8) << 12);
		pos1.z = o.z + (*(int *)(pCurrentCamera + 12) << 12);
	}

	{
		v.vx = e;
		v.vy = f;
		v.vz = g;
		o.x = 0;
		o.y = 0;
		o.z = 0;
		CTC2(0, *(int *)(pCurrentCamera + 0x34));
		CTC2(1, *(int *)(pCurrentCamera + 0x38));
		CTC2(2, *(int *)(pCurrentCamera + 0x3c));
		CTC2(3, *(int *)(pCurrentCamera + 0x40));
		CTC2(4, *(int *)(pCurrentCamera + 0x44));
		MTC2(0, *(int *)&v.vx);
		MTC2(1, *(int *)&v.vz);
		__asm__ volatile("nop; nop; cop2 0x406012");

		STLVNL(&o);

		pos2.x = o.x + (*(int *)(pCurrentCamera + 4) << 12);
		pos2.y = o.y + (*(int *)(pCurrentCamera + 8) << 12);
		pos2.z = o.z + (*(int *)(pCurrentCamera + 12) << 12);
	}

	t = __nw__5CItemUi(412);

	if (t)
		t = CTape_ctor(t, 18, &pos1, &pos2, p, h, ZeroVector, 0, 0xffff);

	t->s400 = 10;
	t->s398 = 10;
	t->s402 = 80;
	t->flags |= 0x200;
	return t;
}
