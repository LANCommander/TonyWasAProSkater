/* WriteNumber__Fi (0x8004c070) */
#include "names.h"

extern char *pNumberStrings;

void WriteNumber(int n) {
	int started = 0;
	int d, q;

	if (n < 0) {
		n = -n;
		*pNumberStrings++ = '-';
	}

	for (d = 1000000000; d > 0; d /= 10) {
		if (n >= d) {
			q = n / d;
			started = 1;
			*pNumberStrings++ = q + '0';
			n -= q * d;
		} else if (started || d == 1) {
			*pNumberStrings++ = '0';
		}
	}

	*pNumberStrings++ = 0;
}
