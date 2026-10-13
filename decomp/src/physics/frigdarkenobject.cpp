/* FrigDarkenObject__6CBruce (0x8003036c) */
/* C++ (PHYSICS.CPP). Awards the combo to the wall item (when its handle is still live): the sum
   of the queued trick scores (mPanel.mTQ) times a multiplier from the trick count and the
   180-spin count, halved, passed to TrickOb_TrickedOnItem. */
#include "game/bruce.h"
#include "game/trickob.h"

void CBruce::FrigDarkenObject() {
	int i, sum, n, score;

	if (!mpWallItem || !Mem_CheckHandle(&mWallHandle))
		return;

	sum = 0;
	n = mPanel.mTQStack;

	for (i = 0; i < n; i++)
		sum += mPanel.mTQ[i].point_value;

	{
		int b = mNum180Spins;
		int n2 = n * 2;

		if (b < 3)
			score = sum * (n2 + b) / 2;
		else {
			int t = n2 - 2;

			score = sum * (t + b * 2) / 2;
		}
	}

	if (score > 0)
		TrickOb_TrickedOnItem(mpWallItem, this, score);
}
