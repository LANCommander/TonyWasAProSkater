/* FlickerSync__Fv (0x80059018) */
#include "names.h"

int DAT_800d1c44;

void VSync(int a);
int FUN_8008cf04(void);

void FlickerSync(void) {
	VSync(0);

	while (FUN_8008cf04() == DAT_800d1c44)
		;

	DAT_800d1c44 ^= 1;
}
