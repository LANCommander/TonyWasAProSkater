/* GetFree256Slot__Fv (0x8004ad9c) */
#include "names.h"

extern unsigned char Free256Slots[];

int GetFree256Slot(void) {
	int i = 0;

	while (!Free256Slots[i]) {
		i++;

		if (i >= 75)
			break;
	}

	Free256Slots[i] = 0;
	return i;
}
