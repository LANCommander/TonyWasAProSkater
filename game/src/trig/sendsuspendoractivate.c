/* SendSuspendOrActivate__FPUsi (0x80045ef4) */
#include "names.h"

extern void *BaddyList, *EnvironmentalObjectList;

extern short **OffsetList;

void SendSuspend(void **pp, int node);
void SendUnSuspend(int node);

void SendSuspendOrActivate(unsigned short *p, int mode) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	for (i = 0; i < n; i++, q++) {
		int node = *q;
		short t = *OffsetList[node];

		if (t == 1 || t == 7) {
			if (mode == 5) {
				SendSuspend(&BaddyList, node);
				SendSuspend(&EnvironmentalObjectList, node);
			} else {
				SendUnSuspend(node);
			}
		}
	}
}
