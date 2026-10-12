/* Move__11CWobblyGlow (0x80029270) */
/* C++ (BIT.CPP). Each section's radius wobbles sinusoidally around the inner radius, each at its own
   rate. */
#include "game/bit.h"

extern short rcossin_tbl[];

void CWobblyGlow::Move() {
	for (uint i = 0; i < mNumSections; i++) {
		mT[i] += mInc[i];
		mpSections[i].Radius = mInnerRadius + mAmplitude * rcossin_tbl[(mT[i] & 0xfff) * 2] / 4096;
	}
}
