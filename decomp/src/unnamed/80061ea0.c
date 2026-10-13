/* ResetOT__Fv (0x80061ea0) */
#include "names.h"

int DAT_800d1d98;
int OT_entry;

void ResetOT(void) {
	DAT_800d1d98 = 0;
	OT_entry = 4095;
}
