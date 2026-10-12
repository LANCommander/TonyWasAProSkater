/* DoHandPlantPhysics__6CBruce (0x80037460) */
/* C++ (PHYSICS.CPP). Handplant: only on the handplant anims (else bail). Holding triangle in
   frames 20..24 ping-pongs the hold (too long bails); at the end, launches back up off the wall
   into the air, flipping the skater if it was the second handplant anim. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

static inline void FlipAxis(short &Matrix, int &Axis) {
	Matrix = -Axis;
	Axis = Matrix;
}

int PingPong(int, int, int);

extern "C" void FUN_8002f438(CBruce *, int);

void CBruce::DoHandPlantPhysics() {
	m70C = 5;

	if (mAnim != 0x22 && mAnim != 0x3f) {
		/* The retail call leaves the limit argument unset; Bail ignores it. */
		Bail(0, DO_LIMIT);
		return;
	}

	if (mAnim < 0x3e && mFrame >= 20 && mFrame < 25) {
		int Hold = m248;

		if (Hold >= 0) {
			if (mpPad->Grind.Pressed) {
				mFrame = PingPong(20, 22, 15);
				FUN_8002f438(this, 20);

				m248++;

				if (m248 >= 21) {
					Bail(0, DO_LIMIT);
					Debug_RunAnim(0x38, 0x1602);

					m24C = 9;
					return;
				}
			} else {
				m248 = -Hold;
			}
		}
	}

	if (mAnimFinished != 0 || (mAnim < 0x3e && mFrame >= 31)) {
		int Hold = m248;

		m740 = 0;
		mVel.vy = (40 - Hold * 3) << 12;
		StateTransition(PHYSICS_ON_GROUND, 0x1614);

		m364 = 1;
		Trick_Land();

		mPos.vx += mUp.vx * 30;
		mPos.vy += mUp.vy * 30;
		mPos.vz += mUp.vz * 30;

		if (mAnim >= 0x3e) {
			mFlipPending = 0;
			mFlipAxesPending = 0;
			mExtraFlags ^= 2;
			FlipAxis(mRotMatrix.m[0][2], mFront.vx);
			FlipAxis(mRotMatrix.m[1][2], mFront.vy);
			FlipAxis(mRotMatrix.m[2][2], mFront.vz);
			FlipAxis(mRotMatrix.m[0][0], mRight.vx);
			FlipAxis(mRotMatrix.m[1][0], mRight.vy);
			FlipAxis(mRotMatrix.m[2][0], mRight.vz);
			Debug_RunAnim(0, 0x162b);
		}
	}
}
