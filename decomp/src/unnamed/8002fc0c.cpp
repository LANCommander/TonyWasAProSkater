/* MaybeFlipAnimationTransition__6CBruce (0x8002fc0c) */
/* C++ (PHYSICS.CPP). Applies a pending flip before an animation change: toggles the flipped flag,
   and/or mirrors the front and right axes (negating the matching rotation matrix columns). */
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

void CBruce::MaybeFlipAnimationTransition() {
	if (mFlipPending) {
		mFlipPending = 0;
		mExtraFlags ^= 2;
	}

	if (mFlipAxesPending) {
		mFlipAxesPending = 0;
		FlipAxis(mRotMatrix.m[0][2], mFront.vx);
		FlipAxis(mRotMatrix.m[1][2], mFront.vy);
		FlipAxis(mRotMatrix.m[2][2], mFront.vz);
		FlipAxis(mRotMatrix.m[0][0], mRight.vx);
		FlipAxis(mRotMatrix.m[1][0], mRight.vy);
		FlipAxis(mRotMatrix.m[2][0], mRight.vz);
	}
}
