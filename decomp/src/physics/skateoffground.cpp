/* SkateOffGround__6CBruce (0x8003434c) */
/* C++ (PHYSICS.CPP). Rolling off an edge into the air: from an invisible surface just changes
   state; otherwise changes state, takes off (Trick_TakeOff), clears the spin tally and trick flag, sets m740 and, unless
   already in the air (mInAir), resets the anim speed and runs the fall anim (26 or 27 by mManIsCrouched). */
#include "game/bruce.h"

void CBruce::SkateOffGround() {
	int v;

	if (mPhysicsState == PHYSICS_ON_INVISIBLE) {
		StateTransition(PHYSICS_IN_AIR, 3707);
	} else {
		StateTransition(PHYSICS_IN_AIR, 3713);
		Trick_TakeOff();

		v = mInAir;
		mTallyAngles = 0;
		mNum180Spins = 0;
		mDoingATrick = 0;
		m740 = 1;

		if (v == 0) {
			mAnimSpeed = 0x10000;

			if (mManIsCrouched)
				Debug_RunAnim(26, 3730);
			else
				Debug_RunAnim(27, 3732);
		}
	}
}
