/* __11CSmokeTrailR7CVectoriiii (0x8002a2ec) */
/* C++ (BIT.CPP). A two-point-per-segment ribbon whose segments are tinted from dark at the tail up
   to (r, g, b) at the head, each fading at 8 per frame. */
#include "game/bit.h"

extern char s_Ribbon[];

CSmokeTrail::CSmokeTrail(CVector &pos, int numbits, int r, int g, int b)
    : CRibbon(pos, numbits, 2, s_Ribbon, s_Ribbon, 400, 1) {
	int dr = r / mNumBits;
	int dg = g / mNumBits;
	int db = b / mNumBits;

	for (int i = 0; i < mNumBits; i++) {
		int steps = mNumBits - (i + 1);

		mBits[i]->SetTint(r - steps * dr, g - steps * dg, b - steps * db);
		mBits[i]->SetTransDecay(8);
	}
}
