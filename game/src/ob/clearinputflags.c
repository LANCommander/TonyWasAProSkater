/* Ob_ClearInputFlags__FP5CBody (0x80044484) */
#include "names.h"

void Ob_ClearInputFlags(char *b) {
	char *n;

	if (b) {
		n = *(char **)(b + 0x1c);

		while (1) {
			*(unsigned short *)(b + 0x50) &= 1;
			b = n;

			if (!b)
				break;

			n = *(char **)(b + 0x1c);
		}
	}
}
