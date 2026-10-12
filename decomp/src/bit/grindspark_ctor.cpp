/* __11CGrindSparkP5CBodyR7CVectoriii (0x80027650) */
/* C++ (BIT2.CPP). Fuzzy demo twin; members and locals from its MAIN.SYM, written as a real constructor.
   The negative-velocity path works on a temporary: assigning the parameter itself moves its prologue copy. */
#include "game/bit.h"
#include "game/ob.h"

int Rnd(int);
void Bit_CalculateSparkVelocity(CVector &, int);

CGrindSpark::CGrindSpark(CBody *pParent, CVector &pos, int vel, int gravity, int life) {
	{
		int vel2;

		if (vel < 0) {
			int t = -vel;

			{
				int r = Rnd(t);

				t >>= 1;
				t += r;
			}

			vel2 = -t;
		} else {
			vel2 = (vel >> 1) + Rnd(vel);
		}

		Bit_CalculateSparkVelocity(mVel, vel2);

		mAcc.vy = gravity;
		mFric.Set(3, 3, 3);
	}

	mLifetime = Rnd(life) + 1;
	mpParent = pParent;
	mOldPos = pos;
	mPos = pos;
	mEnd = pos + mpParent->mPos;
	mStart = mEnd;
	mCodeBGR0 = 0x52000080;

	{
		int r = ((Rnd(159) >> 5) + 2) << 12;

		mPadBGR1 = 0x55000000;
		mCodeBGR0 |= r;
	}
}
