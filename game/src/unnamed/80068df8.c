/* HasSoundCmd__Fv (0x80068df8) */
#include "names.h"

extern int FirstEntry;
extern int LastEntry;

int HasSoundCmd(void) {
	return FirstEntry != LastEntry;
}
