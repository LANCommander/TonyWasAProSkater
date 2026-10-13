/* Ob_MaybeUnSuspendOrCull__Fv (0x800446d4) */
#include "names.h"

typedef struct CBody {
	char _0[4];
	int pos[3];
	char _10[0x0c];
	struct CBody *next;
	char _20[0x32];
	unsigned short flags;
} CBody;

CBody *SuspendedList;
unsigned SuspendDistance;

#ifdef PC_PORT
#include "pc_video.h"

/* objects keep moving as far away as Options > Graphics > Draw Distance shows them */
#define SUSPEND_DISTANCE PC_DrawSuspendDistance(SuspendDistance)
#else
#define SUSPEND_DISTANCE SuspendDistance
#endif

extern CBody *MechList __attribute__((section(".data")));
extern int GNumberOfPlayers __attribute__((section(".data")));
unsigned Utils_CrapDist(int *, int *);
void CBody_UnSuspend(CBody *);

void Ob_MaybeUnSuspendOrCull(void) {
	CBody *b = SuspendedList;
	CBody *n;

	if (!b)
		return;

	n = b->next;

	for (;;) {
		if (b->flags & 2) {
			unsigned d = Utils_CrapDist(MechList->pos, b->pos);

			if (GNumberOfPlayers == 2) {
				unsigned d2 = Utils_CrapDist(MechList->next->pos, b->pos);

				if (d2 < d)
					d = d2;
			}

			if (!(SUSPEND_DISTANCE < d))
				CBody_UnSuspend(b);
		}

		b = n;

		if (n)
			n = n->next;
		else
			break;
	}
}
