/* QueueSoundCmd__F13ESoundCmdTypei (0x80068d98) */
#include "names.h"

typedef struct {
	int type, val;
} SSoundCmd;

extern SSoundCmd SoundCmd[];

extern int LastEntry;

void QueueSoundCmd(int type, int val) {
	SoundCmd[LastEntry].type = type;
	SoundCmd[LastEntry].val = val;
	LastEntry = (LastEntry + 1) % 30;
}
