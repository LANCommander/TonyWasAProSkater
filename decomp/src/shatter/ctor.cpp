/* __11CShatterBitRC8CSVectorN21RC7CVectori (0x80020b84) */
/* C++ (SHATTER.CPP). Demo twin; locals from its MAIN.SYM (two inlined CSVector setters, `this` in
   v1). A shattered-glass chunk: random spin about X or Y, a random lifetime, placed at pos, and
   half the time a smoke trail. THPS1 drops the demo's lifetime * 5 / 6. */
#include "game/bit.h"

int Redbook_Rnd(int);

CShatterBit::CShatterBit(const CSVector &a, const CSVector &b, const CSVector &c,
                         const CVector &pos, int Smoke)
    : CChunkBit(a, b, c) {
	if (Redbook_Rnd(2))
		mAngVel.Set(Redbook_Rnd(160) + 80, 0, 0);
	else
		mAngVel.Set(0, Redbook_Rnd(160) + 80, 0);

	mLifetime = Redbook_Rnd(30) + 45;
	SetPos(pos);
	CalculateWorldCoords();

	if (Redbook_Rnd(2)) {
		mpSmokeTrail = new CSmokeTrail(mPos, 4, 128, 128, 128);
		mpSmokeTrail->mProtected = 1;
		mpSmokeTrail->SetScale(1200);
		mpSmokeTrail->SetTint(32, 32, 32);
	}
}
