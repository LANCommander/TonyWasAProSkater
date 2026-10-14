/* DoPhysics__5CTape (0x8007333c) */
/* C++ (TAPES.CPP). A tape spins at its angular velocity and bobs up and down about its original
   position. */
#include "game/tapes.h"

extern short rcossin_tbl[];

void CTape::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	mPos = mOrgPos;
	mPos.vy += (mWobbleAmplitude * rcossin_tbl[(mT & 0xfff) * 2] / 4096) << 12;
	mT += mWobbleSpeed;
}
