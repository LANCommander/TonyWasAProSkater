/* Die__4CBit (0x80027e18) */
/* C++ (BIT.CPP). Flags the bit dead; the bit manager deletes it later. */
#include "game/bit.h"

void CBit::Die() {
	mDead = 1;
}
