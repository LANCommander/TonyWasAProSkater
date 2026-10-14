/* GetVariable__9CPlatformUs (0x8003f624) */
/* C++ (PLATFORM.CPP). Variable 0x2200 is the platform's not-trodden-upon timer (low half, signed);
   the rest are CBaddy's. */
#include "game/platform.h"

short CPlatform::GetVariable(ushort variable) {
	if (variable == 0x2200)
		return (short)mNotTrodUponTimer;

	return CBaddy::GetVariable(variable);
}
