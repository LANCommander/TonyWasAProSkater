/* FUN_8006ea4c (0x8006ea4c) */
/* C++: a CPanelData member taking the panel as its this (a bail): counts it, ends the replay
   highlight with the queued tricks' value (scaled by the skater's 180 spins) taken off, and starts
   the falling text if a holding score was pending; the trick queue empties. */
#include "game/bruce.h"

void Video_HighlightEnd(CBruce *b, int n);

extern "C" void FUN_8006ea4c(CPanelData *p) {
	int sum = 0;
	int n;
	int i;
	int v;
	int t;

	p->mNumBails += 1;
	n = p->mTQStack;

	if (n > 0) {
		for (i = 0; i < n; i++)
			sum += p->mTQ[i].point_value;

		v = p->mpBruce->mNum180Spins;
		t = n * 2;
		t = n * 2;

		{
			int k = v;

			if (k < 3)
				v = t + k;
			else {
				int u = t - 2;

				v = u + k * 2;
			}
		}

		sum = sum * v / 2;
	}

	Video_HighlightEnd(p->mpBruce, -sum);

	if (p->mLastHoldingScore) {
		p->mLastHoldingScore = 0;
		p->mLastMult = 1;
		p->mFallingTextEffect.init();
	}

	p->mTQStack = 0;
}
