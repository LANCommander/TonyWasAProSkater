/* SendSignalToLinks__FPUs (0x80045d70) */
#include "names.h"

extern void *BaddyList, *EnvironmentalObjectList;

extern short **OffsetList;

void SendSignalToNode(void *b, int node);

void SendSignalToLinks(unsigned short *p) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	for (i = 0; i < n; i++) {
		int node = *q;
		short t = *OffsetList[node];

		if (t == 1 || t == 7) {
			SendSignalToNode(BaddyList, node);
			SendSignalToNode(EnvironmentalObjectList, node);
		}

		q++;
	}
}
