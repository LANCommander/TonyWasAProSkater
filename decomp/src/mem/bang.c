/* Bang__FUcUcUc (0x8004ce28) */
#include "names.h"

char DAT_800d1b38[8];

void ClearImage(char *, int, int, int);

void Bang(unsigned char a, unsigned char b, unsigned char c) {
	ClearImage(DAT_800d1b38, a, b, c);

	for (;;)
		;
}
