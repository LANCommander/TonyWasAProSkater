/* SendSuspend__FPP5CBodyi (0x80045e30) */
#include "game/ob.h"

void SendSuspend(CBody **pp, int node) {
	CBody *b;

	for (b = *pp; b; b = (CBody *)b->mNextItem)
		if (b->mNode == node)
			b->Suspend(pp);
}
