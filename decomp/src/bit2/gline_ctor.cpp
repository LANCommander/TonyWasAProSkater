/* __6CGLine (0x80027590) */
/* C++ (BIT2.CPP). Demo twin. */
#include "game/bit.h"

extern char GLineList[];

CGLine::CGLine() {
	mCodeBGR0 = 0x50808080;
	mPadBGR1 = 0x55808080;
	AttachTo(GLineList);
}
