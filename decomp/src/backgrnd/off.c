/* Backgrnd_Off__FUs (0x8007813c) */
#include "names.h"

char *BackgroundList;

void Backgrnd_Off(unsigned short id) {
	char *p;

	for (p = BackgroundList; p; p = *(char **)(p + 0x1c)) {
		if (*(unsigned short *)(p + 0x134) == id) {
			*(unsigned short *)p |= 1;
			return;
		}
	}
}
