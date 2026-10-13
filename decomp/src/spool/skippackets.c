/* Spool_SkipPackets__FPUi (0x8006f130) */
#include "names.h"

unsigned *Spool_SkipPackets(unsigned *p) {
	unsigned *q = (unsigned *)((char *)p + p[1]);

	while (*q != -1) {
		unsigned s;

		q++;
		s = *q++;
		q = (unsigned *)((char *)q + s);
	}

	return q + 1;
}
