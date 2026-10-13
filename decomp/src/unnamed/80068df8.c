/* HasSoundCmd__Fv (0x80068df8) */
#include "names.h"

int FirstEntry;
int LastEntry;

int HasSoundCmd(void) {
	return FirstEntry != LastEntry;
}
