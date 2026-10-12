/* DeleteStuff__5CTape (0x800732e4) */
/* C++ (TAPES.CPP). Deletes the tape's glow. */
#include "game/tapes.h"
#include "game/bit.h"

void CTape::DeleteStuff() {
	delete mpGlow;
	mpGlow = 0;
}
