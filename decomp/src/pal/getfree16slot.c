/* GetFree16Slot__Fv (0x8004ace8) */
#include "names.h"

extern unsigned char Free16Slots[];

int GetFree16Slot(void) {
	int i = 0;

	while (!Free16Slots[i]) {
		i++;

		if (i >= 256)
			break;
	}

	Free16Slots[i] = 0;
	return i;
}
