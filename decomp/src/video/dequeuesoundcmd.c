/* DequeueSoundCmd__FR13ESoundCmdTypeRi (0x80068e10) */
#include "names.h"

typedef struct {
	int type, val;
} SSoundCmd;

extern SSoundCmd SoundCmd[];

int FirstEntry;

void DequeueSoundCmd(int *type, int *val) {
	*type = SoundCmd[FirstEntry].type;
	*val = SoundCmd[FirstEntry].val;
	FirstEntry = (FirstEntry + 1) % 30;
}
