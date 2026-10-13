/* Trig_SendPulse__FPUs (0x80045718) */
#include "names.h"

void Trig_SendPulseToNode(int n);

void Trig_SendPulse(unsigned short *p) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	i = 0;

	while (i < n) {
		i++;
		Trig_SendPulseToNode(*q++);
	}
}
