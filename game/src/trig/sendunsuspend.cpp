/* SendUnSuspend__Fi (0x80045e98) */
#include "game/ob.h"

extern CBody *SuspendedList;

void SendUnSuspend(int node) {
	CBody *b;

	for (b = SuspendedList; b; b = (CBody *)b->mNextItem)
		if (b->mNode == node)
			b->UnSuspend();
}
