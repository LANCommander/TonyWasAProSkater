/* Die__6CBaddy (0x8003fb1c) */
/* C++ (BADDY.CPP). Kills the baddy once: sends its death pulse, marks it dead and hides it. */
#include "game/baddy.h"

void CBaddy::Die() {
	if (!IsDead()) {
		SendDeathPulse();

		mCBodyFlags |= 0x40;
		mFlags |= 0x41;
	}
}
