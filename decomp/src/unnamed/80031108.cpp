/* HandleBailOnLanding__6CBrucei (0x80031108) */
/* C++ (PHYSICS.CPP). On landing: bails if a held trick button is still down, if not upright, or
   if a rail landing is too far off line; counts it as a trick bail. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::HandleBailOnLanding(int) {
	if (mInAir)
		return;

	if (!mDoingATrick && !m720)
		return;

	int Bailing = 0;

	if (m368 & 0x2000)
		Bailing = mpPad->Flip.Pressed != 0;

	if (m368 & 0x4000)
		Bailing |= mpPad->Grab.Pressed != 0;

	Bailing |= m360 == 0;

	if (m720) {
		if (m728 > 0) {
			if ((m724 & 4095) < 3796)
				Bailing = 1;
		} else {
			if ((m724 & 4095) >= 301)
				Bailing = 1;
		}
	}

	if (Bailing) {
		/* The retail call leaves the limit argument unset; Bail ignores it. */
		Bail(0, DO_LIMIT);

		mTrickBails += 1;
	}
}
