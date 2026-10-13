/* CalcChecksum__FPUlUl (0x800142c4) */
#include "names.h"

int CalcChecksum(unsigned char *p, unsigned n) {
	unsigned i;
	short s = 0;

	for (i = 0; i < n; i++)
		s = s + (p[i] + 1);

	return s;
}
