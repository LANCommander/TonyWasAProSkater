/* Free256Slot__FUs (0x8004adec) */
#include "names.h"

extern unsigned char Free256Slots[];

extern int Pal256X, Pal256Y;

unsigned short FUN_8008f70c(int, int);

void Free256Slot(unsigned short clut) {
	int i;

	for (i = 0; i < 75; i++)
		if (FUN_8008f70c(Pal256X, Pal256Y + i) == clut)
			break;

	Free256Slots[i] = 1;
}
