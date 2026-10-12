/* BoardOff__6CBruce (0x80041c30) */
/* C++ (MECH.CPP). Hides the board and both wheel models of this skater. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];

void CBruce::BoardOff() {
	SStats *pStats = &Stats[mSkaterIndex];

	mSubObjectDisplayMask |= (1 << pStats->Board) | (1 << pStats->Wheel1) | (1 << pStats->Wheel2);
}
