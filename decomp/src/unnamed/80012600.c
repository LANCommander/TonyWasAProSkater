/* NameWasBlank__Fv (0x80012600) */
#include "names.h"

extern unsigned char SaveGameName[];

int NameWasBlank(void) {
	int i;

	for (i = 0; i < 10; i++) {
		if (SaveGameName[i] == 0)
			return 1;

		if (SaveGameName[i] != 32)
			return 0;
	}

	return 1;
}
