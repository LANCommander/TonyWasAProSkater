/* SetOTPlayer__Fi (0x80061eb4) */
#include "names.h"

extern int DAT_800d1d98;
extern int OT_entry;

void SetOTPlayer(int a) {
	if (DAT_800d1d98 != a) {
		if (a == 0)
			OT_entry -= 4096;
		else
			OT_entry += 4096;

		DAT_800d1d98 = a;
	}
}
