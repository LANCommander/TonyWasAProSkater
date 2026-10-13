/* PadBuffer_SetNumPlayers__Fi (0x800642d4) */
#include "names.h"

int DAT_800d1d9c;

void PadBuffer_SetNumPlayers(int a) {
	DAT_800d1d9c = a == 1;
}
