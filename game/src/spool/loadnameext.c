/* LoadNameExt__FPcT0 (0x80070140) */
#include "names.h"

extern char SpoolQ[][17];

int FreePos;

void LoadNameExt(char *a, char *b) {
	char *d = SpoolQ[FreePos];
	char *s = a;
	char *t = b;

	while (*s)
		*d++ = *s++;

	while (*t)
		*d++ = *t++;

	*d = 0;
}
