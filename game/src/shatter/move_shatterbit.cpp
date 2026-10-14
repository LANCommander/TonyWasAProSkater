/* Move__11CShatterBit (0x80020e8c) */
/* C++ (SHATTER.CPP). A glass chunk flies and spins under gravity, drags its smoke trail along and
   dies when its lifetime runs out. */
#include "game/bit.h"

void CShatterBit::Move() {
	mPos += mVel;
	mVel.vy += 0x5014;
	mAngles += mAngVel;

	if (mpSmokeTrail)
		mpSmokeTrail->SetPos(mPos);

	CalculateWorldCoords();

	if (mLifetime)
		mLifetime--;
	else
		Die();
}
