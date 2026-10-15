/* __7CRibbonRC7CVectoriiPcT4ii (0x80029e28) */
/* C++ (BIT.CPP). Fuzzy demo twin; members and locals from its MAIN.SYM. A real constructor, with the
   parameters left unmodified so the prologue copies them in retail's order. */
#include "game/bit.h"

void *Mem_New(unsigned int, int, int);

CRibbon::CRibbon(const CVector &pos, int numbits, int pointsperbit, char *pMiddleAnimName,
                 char *pEndAnimName, int scale, int semitrans) {
	mNumBits = numbits;
	mPointsPerBit = pointsperbit - 1;
	mNumPoints = numbits * mPointsPerBit + 1;
	mPoints = (CVector *)Mem_New(mNumPoints * sizeof(CVector), 0, 1);

	{
		int i;

		for (i = 0; i < mNumPoints; i++)
			mPoints[i] = pos;
	}

	{
		int i;

		mBits = (CRibbonBit **)Mem_New(numbits * sizeof(CRibbonBit *), 0, 1);

		for (i = mNumBits - 1; i >= 0; i--) {
			mBits[i] = new CRibbonBit;
			mBits[i]->mProtected = 1;
			mBits[i]->SetAnim(pMiddleAnimName);
			mBits[i]->SetScale(scale);

			if (semitrans)
				mBits[i]->SetSemiTransparent();
		}
	}

	mBits[0]->mBitFlags |= 16;
	mBits[0]->SetAnim(pEndAnimName);
}
