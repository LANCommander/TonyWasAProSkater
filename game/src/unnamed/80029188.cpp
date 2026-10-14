/* __11CWobblyGlowRC7CVectoriiiUcUcUcUcUcUc (0x80029188) */
/* C++ (BIT.CPP). A glow whose section radii wobble around InnerRadius by Wobble/256 of it, each
   section starting at a random phase with a random rate. */
#include "game/bit.h"

int Redbook_Rnd(int);

CWobblyGlow::CWobblyGlow(const CVector &Pos, int InnerRadius, int FringeRadius, int Wobble,
                         uchar r0, uchar g0, uchar b0, uchar r1, uchar g1, uchar b1)
    : CGlow(Pos, InnerRadius, FringeRadius, r0, g0, b0, r1, g1, b1) {
	mInnerRadius = InnerRadius;
	mAmplitude = (InnerRadius * Wobble) / 256;

	for (uint i = 0; i < mNumSections; i++) {
		mT[i] = Redbook_Rnd(4096);
		mInc[i] = Redbook_Rnd(50) + 50;
	}
}
