/* Ob_MaybeUnSuspendOrCull__Fv (0x800446d4) */
#include "game/ob.h"

extern CBody *SuspendedList;
extern unsigned SuspendDistance;

#include "pc_video.h"

/* objects keep moving as far away as Options > Graphics > Draw Distance shows them */
#define SUSPEND_DISTANCE PC_DrawSuspendDistance(SuspendDistance)

extern CBody *MechList __attribute__((section(".data")));
extern int GNumberOfPlayers __attribute__((section(".data")));
ulong Utils_CrapDist(const CVector &, const CVector &);

void Ob_MaybeUnSuspendOrCull() {
	CBody *b = SuspendedList;
	CBody *n;

	if (!b)
		return;

	n = (CBody *)b->mNextItem;

	for (;;) {
		if (b->mCBodyFlags & 2) {
			unsigned d = Utils_CrapDist(MechList->mPos, b->mPos);

			if (GNumberOfPlayers == 2) {
				unsigned d2 = Utils_CrapDist(MechList->mNextItem->mPos, b->mPos);

				if (d2 < d)
					d = d2;
			}

			if (!(SUSPEND_DISTANCE < d))
				b->UnSuspend();
		}

		b = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}
