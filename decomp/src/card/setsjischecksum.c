/* SetSJISChecksum__FPcT0 (0x8001424c) */
#include "names.h"

void SetSJISChecksum(unsigned short *src, char *dst) {
	unsigned short h = 0;
	unsigned a, b;

	while (*src) {
		h = ((h << 2) + (h >> 10)) ^ (*src >> 8);
		src++;
	}

	h &= 0x3ff;
	b = h & 31;
	h >>= 5;

	if (h < 26)
		dst[0] = h + 65;
	else
		dst[0] = h + 22;

	if (b < 26)
		dst[1] = b + 65;
	else
		dst[1] = b + 22;
}
