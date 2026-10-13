/* KillInList__FiP5CBodyi (0x80045774) */
/* C++ (TRIG). Sends a trigger event to every body in the list whose node id matches: mode 0 kills it
   (Die, virtual slot 1), mode 1 hits it (Hit, slot 5) with its mHealth. */
#include "game/ob.h"

extern CVector ZeroVector;

void KillInList(int id, CBody *pBody, int mode) {
	for (; pBody; pBody = (CBody *)pBody->mNextItem) {
		if (pBody->mNode != id)
			continue;

		switch (mode) {
			case 0:
				pBody->Die();
				break;
			case 1:
				pBody->Hit(pBody->mHealth, ZeroVector, 0);
				break;
		}
	}
}
