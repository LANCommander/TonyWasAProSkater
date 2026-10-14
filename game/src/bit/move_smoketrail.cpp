/* Move__11CSmokeTrail (0x8002a49c) */
/* C++ (BIT.CPP). Once the trail is fading away, fades every segment and dies when all have gone. */
#include "game/bit.h"

void CSmokeTrail::Move() {
	if (mFadeAway) {
		int dead = 1;

		for (int i = 0; i < mNumBits; i++)
			if (!mBits[i]->Fade(0))
				dead = 0;

		if (dead)
			Die();
	}
}
