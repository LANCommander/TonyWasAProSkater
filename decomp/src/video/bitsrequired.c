/* BitsRequired__Fl (0x80068ae0) */
#include "names.h"

int BitsRequired(long x) {
	int n = 2;
	int m;

	if (x < 0) {
		for (m = -2; x < m; m <<= 1)
			n++;
	} else {
		for (m = 2; x >= m; m <<= 1)
			n++;
	}

	return n;
}
