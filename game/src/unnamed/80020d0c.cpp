/* SetPos__11CShatterBitRC7CVector (0x80020d0c) */
/* C++ (SHATTER.CPP). Places a glass chunk and throws it out from the shattered face's centre, plus a
   random push along the face normal and upwards (sometimes doubled); never downwards. */
#include "game/bit.h"

extern CSVector *pNormal;

extern CVector DAT_800d2574; /* centre of the face being shattered */

int Redbook_Rnd(int);

void CShatterBit::SetPos(const CVector &Pos) {
	CBit::SetPos(Pos);

	CVector Dir = Pos - DAT_800d2574;
	int Len = Dir.Length();

	if (Len) {
		int Speed = Redbook_Rnd(48);

		mVel = (Speed * Dir) / Len;
	} else {
		Die();
	}

	int Push = Redbook_Rnd(45);

	mVel.vx += Push * pNormal->vx;
	mVel.vy += Push * pNormal->vy;
	mVel.vz += Push * pNormal->vz;
	mVel.vy -= Redbook_Rnd(175) << 12;

	if (Redbook_Rnd(20) == 0)
		mVel <<= 1;

	if (mVel.vy > 0)
		mVel.vy = 0;

	if (mpSmokeTrail)
		mpSmokeTrail->SetPos(mPos);
}
