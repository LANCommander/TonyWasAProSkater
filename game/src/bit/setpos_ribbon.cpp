/* SetPos__7CRibbonR7CVector (0x8002a1e8) */
/* C++ (BIT.CPP). Pushes a new head position into the ribbon's ring of points and re-spans every
   segment between points mPointsPerBit apart. */
#include "game/bit.h"

void CRibbon::SetPos(CVector &pos) {
	int j;

	mCurrent++;

	if (mCurrent == mNumPoints)
		mCurrent = 0;

	mPoints[mCurrent] = pos;
	j = mCurrent;

	for (int i = mNumBits - 1; i >= 0; i--) {
		mBits[i]->mEndPosB = mPoints[j];
		j -= mPointsPerBit;

		if (j < 0)
			j += mNumPoints;

		mBits[i]->mEndPosA = mPoints[j];
	}
}
