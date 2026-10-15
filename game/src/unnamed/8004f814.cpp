/* _._5CRail (0x8004f814) flags=-fstructure-noalias */
/* C++ (RAIL.CPP). Unlinks the rail from RailList. */
#include "game/trickob.h"

extern CRail *RailList;

CRail::~CRail() {
	if (RailList == this)
		RailList = mpNext;

	if (mpPrevious)
		mpPrevious->mpNext = mpNext;

	if (mpNext)
		mpNext->mpPrevious = mpPrevious;
}
