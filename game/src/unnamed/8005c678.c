/* ClearSoftReset__Fv (0x8005c678) */
#include "names.h"

extern int DAT_800d1cb8;
extern int DAT_800d2348;

void ClearSoftReset(void) {
	DAT_800d1cb8 = 0;
	DAT_800d2348 = 110;
}
