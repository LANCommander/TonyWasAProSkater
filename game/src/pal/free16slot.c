/* Free16Slot__FUs (0x8004ad38) */
#include "names.h"

extern unsigned char Free16Slots[];

int Pal16X, Pal16Y;

unsigned short FUN_8008f70c(int, int);

void Free16Slot(unsigned short clut) {
	int i;

	for (i = 0; i < 256; i++)
		if (FUN_8008f70c(Pal16X, Pal16Y + i) == clut)
			break;

	Free16Slots[i] = 1;
}
