/* Get256SlotNumber__FUs (0x8004ae50) */
#include "names.h"

int Pal256X, Pal256Y;

unsigned short FUN_8008f70c(int, int);

int Get256SlotNumber(unsigned short clut) {
	int i;

	for (i = 0; i < 75; i++)
		if (FUN_8008f70c(Pal256X, Pal256Y + i) == clut)
			break;

	return i;
}
