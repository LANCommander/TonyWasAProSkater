/* StateTransition__6CBruce13EPhysicsStatei (0x8002f980) */
/* C++ (PHYSICS.CPP). Changes physics state: clears rail state on leaving a rail, counts airs,
   remembers the wall when going onto an invisible wall, and leaving a wallride scores it and
   turns the skater's right/up axes back off the wall. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

extern char s_WALL_RIDE[];

int Mips_FixMul(int, int);

void CBruce::StateTransition(EPhysicsState NewState, int) {
	CVector Old;

	if (mPhysicsState == PHYSICS_ON_RAIL && NewState == PHYSICS_IN_AIR) {
		m264 = 0;
		m260 = 0;
		m280 = 0;
		m27C = 0;
		m270 = 0;
		m274 = 0;
		m26C = 0;
	}

	if (mPhysicsState != NewState) {
		if (NewState == PHYSICS_IN_AIR && mPhysicsState != PHYSICS_ON_INVISIBLE)
			mAirs++;

		if (NewState == PHYSICS_ON_INVISIBLE && mPhysicsState != PHYSICS_IN_AIR)
			mAirs++;
	}

	if (mPhysicsState != PHYSICS_ON_INVISIBLE && NewState == PHYSICS_ON_INVISIBLE) {
		mFallLine = mFront;
		mFallLine.vx = -Mips_FixMul(mFallLine.vx, 13000);
		mFallLine.vy = Mips_FixMul(mFallLine.vy, 13000);
		mFallLine.vz = -Mips_FixMul(mFallLine.vz, 13000);
	}

	if (mPhysicsState == PHYSICS_IN_WALLRIDE && NewState == PHYSICS_IN_AIR) {
		Trick_Trick(64, s_WALL_RIDE, 500, 0);

		int Flipped = (mExtraFlags & 2) != 0;

		if (mAnim == 35 ? !Flipped : Flipped) {
			Old = mRight;
			mRight = mUp;
			mUp = -Old;
		} else {
			Old = mRight;
			mRight = -mUp;
			mUp = Old;
		}
	}

	RotMatrixFromFRUVectors();

	mPhysicsState = NewState;
}
