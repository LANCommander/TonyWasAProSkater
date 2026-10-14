/* DoPhysics__6CMedal (0x80073cf8) */
#include "game/medals.h"

struct SSinCos {
	short s, c;
};

extern SSinCos rcossin_tbl[];

#define SIN(a) (rcossin_tbl[(a) & 4095].s)

void CMedal::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	mPos = mOrgPos;
	mPos.vy += ((mWobbleAmplitude * SIN(mT)) / 4096) << 12;
	mT += mWobbleSpeed;
}
