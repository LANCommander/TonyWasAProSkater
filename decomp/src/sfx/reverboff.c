/* SFX_ReverbOff__Fv (0x8004bafc) */
#include "names.h"

extern void SFX_SetReverbType(unsigned char);

void SFX_ReverbOff(void) {
	SFX_SetReverbType(0);
}
