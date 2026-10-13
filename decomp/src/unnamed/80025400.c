/* FPS_Init__Fv (0x80025400) */
#include "names.h"

extern int DAT_800a7214[];
extern int Xblanks;

int LastXblanks;

void FPS_Init(void) {
	int i;

	for (i = 0; i < 3; i++)
		DAT_800a7214[i] = 2;

	LastXblanks = Xblanks - 2;
}
