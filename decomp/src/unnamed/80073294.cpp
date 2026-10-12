/* DontDisplay__5CTape (0x80073294) */
/* C++ (TAPES.CPP). Hides the tape (flags 0x41) and deletes its glow (virtual DeleteStuff). */
#include "game/tapes.h"

void CTape::DontDisplay() {
	mFlags |= 0x41;
	DeleteStuff();
}
