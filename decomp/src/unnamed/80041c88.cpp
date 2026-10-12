/* BoardOn__6CBruce (0x80041c88) */
/* C++ (MECH.CPP). Shows the board and both wheel models of this skater again. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];

void CBruce::BoardOn() {
	SStats *pStats = &Stats[mSkaterIndex];

	mSubObjectDisplayMask &= ~((1 << pStats->Board) | (1 << pStats->Wheel1) | (1 << pStats->Wheel2));
}
