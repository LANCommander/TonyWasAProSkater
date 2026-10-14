/* Die__6CBruce (0x80042778) */
/* C++ (MECH.CPP). Kills the skater unless already dead: flags the body and item as dead. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::Die() {
	if (!IsDead()) {
		mCBodyFlags |= 0x40;
		mFlags |= 0x41;
	}
}
