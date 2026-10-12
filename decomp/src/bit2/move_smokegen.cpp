/* Move__15CSmokeGenerator (0x80027450) */
/* C++ (BIT2.CPP). Demo twin; locals from its MAIN.SYM. Spawns mPuffs motion-blur smoke bits rising
   at a random speed, then ages the generator and kills it when its life runs out. The CVector
   inline constructor and the CBit/CFlatBit inline setters (int params) give retail's prologue and
   the hoisted `100` in s4. */
#include "game/bit.h"

int Rnd(int n);

extern char s_Smoke[];

void CSmokeGenerator::Move() {
	CVector upvel(0, 0, 0);
	int i;

	for (i = 0; i < mPuffs; i++) {
		CMotionBlur *foo;

		upvel.vy = -(mVBase + Rnd(mVRandom)) << 12;
		foo = new CMotionBlur(mPos, upvel, s_Smoke, mScaleBase + Rnd(mScaleRandom), 0, 10);

		foo->SetFrigDeltaZ(100);
		foo->SetAngle(Rnd(4096));
		foo->SetTint(mR, mG, mB);
	}

	{
		short l;

		mAge++;
		l = mAge;

		if (l == mLifetime && (ushort)l != 0xffff)
			Die();
	}
}
