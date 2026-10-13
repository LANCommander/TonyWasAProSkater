/* HandleHoldTrick__6CBruce (0x80031500) */
/* C++ (PHYSICS.CPP). While a held trick is being scored (m368 & 0x800, trick in the low byte,
   0x4000/0x2000 states), adds 1/20 of its depreciated table score via Trick_HoldTrick. */
#include "game/bruce.h"

extern STrick Group[];

void CBruce::HandleHoldTrick() {
	int f = m368;
	int n = 0;

	if (f & 0x800) {
		int t = f & 0xff;

		if (f & 0x6000)
			n = Trick_RepeatScore(t, Group[t].Score) / 20;

		if (n)
			Trick_HoldTrick(n);
	}
}
