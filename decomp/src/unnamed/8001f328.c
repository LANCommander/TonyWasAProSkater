/* Flash_FadeFinished__Fv (0x8001f328) */
#include "names.h"

int FadeCountdown;

int Flash_FadeFinished(void) {
	return FadeCountdown == 0;
}
