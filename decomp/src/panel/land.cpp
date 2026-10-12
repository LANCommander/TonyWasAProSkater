/* Land__10CPanelData (0x8006e8ec) */
/* C++ (PANEL.CPP). Demo twin. Banks a landed combo: sums the trick scores, applies the multiplier
   (combo length plus a spin bonus), keeps the best combo's names, and ends the replay highlight. */
#include "game/panel.h"
#include "game/bruce.h"
#include "game/career.h"

extern SCareer GCareer;
extern int GGame;

extern "C" char *FUN_8008e904(char *, char *); /* strcpy */
void Video_HighlightEnd(CBruce *, int);

void CPanelData::Land() {
	mLastHoldingScore = 0;
	mLastMult = 0;

	if (mTQStack > 0) {
		int spins, mult;
		uint score;

		mLandedScore = 0;
		mNumTricksLanded += mTQStack;

		for (int i = 0; i < mTQStack; i++)
			mLandedScore += mTQ[i].point_value;

		spins = mpBruce->mNum180Spins;

		if (spins < 3)
			mult = mTQStack * 2 + spins;
		else
			mult = mTQStack * 2 + (spins * 2 - 2);

		mLandedMultiplier = mult;
		score = (mLandedScore * mult) >> 1;
		mLandedTotal += score;

		if (mBestComboScore < score) {
			mBestComboScore = score;
			mNumTricksInCombo = mTQStack;
			FUN_8008e904(mBestComboName, mTrickName);
			FUN_8008e904(mBestComboName2, mTrickName2);
			FUN_8008e904(mBestComboName3, mTrickName3);
		}

		mTQStack = 0;
		m4F4 = 40;

		if (GCareer.Options.ScoreDisplayEnabled == 0)
			m4F4 = 0;

		if (GGame == 2 && mLandedScore != 0)
			mTotalScore = 0;
	}

	Video_HighlightEnd(mpBruce, mLandedTotal);
}
