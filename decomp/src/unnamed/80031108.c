/* HandleBailOnLanding__6CBrucei (0x80031108) */
#include "names.h"
void CBruce_Bail(char *p, int a);

void CBruce_HandleBailOnLanding(char *p, int n) {
	int a0, v1, a1, a2;

	if (*(int *)(p + 856))
		return;

	if (!*(int *)(p + 860)) {
		a1 = *(int *)(p + 1824);

		if (!a1)
			return;
	}

	a2 = *(int *)(p + 872);
	a1 = *(int *)(p + 1824);
	v1 = 0;

	if (a2 & 8192)
		v1 = *(unsigned char *)(*(int *)(p + 2156) + 16) != 0;

	a0 = v1;
	v1 = 0;

	if (a2 & 16384)
		v1 = *(unsigned char *)(*(int *)(p + 2156) + 32) != 0;

	a0 |= v1;
	a0 |= (*(int *)(p + 864) == 0);

	if (a1) {
		if (*(int *)(p + 1832) > 0) {
			if ((*(int *)(p + 1828) & 4095) < 3796)
				a0 = 1;
		} else {
			if (!((*(int *)(p + 1828) & 4095) < 301))
				a0 = 1;
		}
	}

	if (a0) {
		CBruce_Bail(p, 0);

		*(int *)(p + 544) += 1;
	}
}
