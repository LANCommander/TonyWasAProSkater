/* Ob_AI__FPP5CBody (0x800444bc) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

typedef struct O {
	char p0[4];
	V pos;
	char p1[0x1c - 0x10];
	struct O *next;
	char p2[0x48 - 0x20];
	struct VT *vt;
	char p3[0x52 - 0x4c];
	unsigned short f52;
	char p4[0xe8 - 0x54];
	unsigned fe8;
} O;

typedef struct {
	char p[4];
	V pos;
	char q[0x1c - 0x10];
	O *f1c;
} P;

extern P *MechList;
extern int GNumberOfPlayers;

unsigned SuspendDistance;

#include "pc_video.h"

/* objects keep moving as far away as Options > Graphics > Draw Distance shows them */
#define SUSPEND_DISTANCE PC_DrawSuspendDistance(SuspendDistance)

void CItem_FlashUpdate(O *);
void CBody_Suspend(O *, O **);
void FUN_800440ec(O *);
void CBody_UpdateShadow(O *);
unsigned Utils_CrapDist(V *, V *);

void Ob_AI(O **a) {
	O *m;
	O *n;
	V *v;
	unsigned d, t;

	m = *a;

	if (!m)
		return;

	n = m->next;

	for (;;) {
		CItem_FlashUpdate(m);

		if (m->f52 & 0x40) {
			if (m->f52 & 0x80) {
				if (m)
					m->vt->fn((char *)m + m->vt->off, 3);
			} else
				m->f52 = m->f52 | 0x80;
		} else {
			v = &m->pos;
			d = Utils_CrapDist(&MechList->pos, v);

			if (GNumberOfPlayers == 2) {
				t = Utils_CrapDist(&MechList->f1c->pos, v);

				if (t < d)
					d = t;
			}

			m->fe8 = d;

			if ((m->f52 & 2) && SUSPEND_DISTANCE < d) {
				CBody_Suspend(m, a);
			} else {
				FUN_800440ec(m);
				CBody_UpdateShadow(m);
			}
		}

		m = n;

		if (n)
			n = n->next;
		else
			break;
	}
}
