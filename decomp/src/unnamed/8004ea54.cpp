/* __11CFlatRippleRC7CVectoriUcUcUcii (0x8004ea54) */
/* C++ (EXP.CPP). A ripple: a semi-transparent poly-line ring of NumSegs segments, all at Pos in
   colour (r, g, b), that will grow at GrowRate up to MaxRadius from a random angle. */
#include "game/bit.h"
#include "game/m3d.h"

int Redbook_Rnd(int);

CFlatRipple::CFlatRipple(const CVector &Pos, int MaxRadius, uchar r, uchar g, uchar b, int NumSegs,
                         int GrowRate)
    : CPolyLine(NumSegs) {
	mPos = Pos;
	mMaxRadius = MaxRadius;
	mGrowRate = GrowRate;
	mStart = Pos;
	mR = r;
	mG = g;
	mB = b;

	for (int i = 0; i < mNumSegs; i++) {
		mSegs[i].r = r;
		mSegs[i].g = g;
		mSegs[i].b = b;
		mSegs[i].End = Pos;
	}

	mAngle = Redbook_Rnd(4096);
	SetSemiTransparent();
}
