/* KillShadow__5CBody (0x80043af4) */
/* C++ (OB.CPP). Turns the body's shadow off and deletes its shadow bit. */
#include "game/ob.h"
#include "game/bit.h"

void CBody::KillShadow() {
	mCBodyFlags &= ~8;

	if (mpShadow) {
		delete mpShadow;

		mpShadow = 0;
	}
}
