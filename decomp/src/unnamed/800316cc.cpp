/* SetTrickFlags__6CBruceP6STrick (0x800316cc) */
/* PHYSICS: copies a trick-table entry's flag bits (top 12 bits of its first word) into the
   skater; called twice from HandleTricksNew__6CBruce with &Group[i]. */
#include "game/bruce.h"

void CBruce::SetTrickFlags(STrick *pTrick) {
	unsigned Flags = pTrick->Flags & 0xfff00000;

	mNoSpin = (Flags & 0x100000) != 0;
	mNoSpinScore = (Flags & 0x200000) != 0;
	m1EC = (Flags & 0x4000000) != 0;

	if (mNoSpin)
		mTapTurn = 0;

	if (Flags & 0x800000) {
		m368 = (m368 & 0xffffff) | 0x80000000;
		m368 &= ~0x4000;
		m368 |= 0x2000;
	}

	mFlipPending = (Flags & 0x1000000) != 0;
	mFlipAxesPending = (Flags & 0x400000) != 0;
}
