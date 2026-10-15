/* Ob_AI__FPP5CBody (0x800444bc) */
/* C++ (OB.CPP): runs a body list's AI: flash update, deferred deletes (flag 0x40, done on the
   second frame), the distance to the nearest skater, then suspend or AI + shadow. */
#include "game/ob.h"

extern CBody *MechList[2];
extern int GNumberOfPlayers;

unsigned SuspendDistance;

#include "pc_video.h"

/* objects keep moving as far away as Options > Graphics > Draw Distance shows them */
#define SUSPEND_DISTANCE PC_DrawSuspendDistance(SuspendDistance)

extern "C" void FUN_800440ec(CBody *);
unsigned Utils_CrapDist(const CVector &, const CVector &);

void Ob_AI(CBody **a) {
	CBody *m;
	CBody *n;
	CVector *v;
	unsigned d, t;

	m = *a;

	if (!m)
		return;

	n = (CBody *)m->mNextItem;

	for (;;) {
		m->FlashUpdate();

		if (m->mCBodyFlags & 0x40) {
			if (m->mCBodyFlags & 0x80) {
				if (m)
					delete m;
			} else
				m->mCBodyFlags = m->mCBodyFlags | 0x80;
		} else {
			v = &m->mPos;
			d = Utils_CrapDist(MechList[0]->mPos, *v);

			if (GNumberOfPlayers == 2) {
				t = Utils_CrapDist(MechList[0]->mNextItem->mPos, *v);

				if (t < d)
					d = t;
			}

			m->mPlayerDist = d;

			if ((m->mCBodyFlags & 2) && SUSPEND_DISTANCE < d) {
				m->Suspend(a);
			} else {
				FUN_800440ec(m);
				m->UpdateShadow();
			}
		}

		m = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}
