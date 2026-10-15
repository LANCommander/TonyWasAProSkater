/* SendSignalToNode__FP5CBodyi (0x80045d30) */
/* C++. Sets input flag bit 0 on every body in the list attached to the given node. */
#include "game/ob.h"

void SendSignalToNode(CBody *b, int node) {
	while (b) {
		if (b->mNode == node)
			b->mInputFlags |= 1;

		b = (CBody *)b->mNextItem;
	}
}
