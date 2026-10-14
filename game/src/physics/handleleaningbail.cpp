/* HandleLeaningBail__6CBruceG7SVECTOR (0x80035f30) */
/* C++ (PHYSICS.CPP). On the ground, outside a manual: if the skater leans too far from the surface
   normal `n` (mUp . n < 2000) and is not upside down, bails, runs the leaning-bail anim, swaps the
   front/up vectors (front = -up, up = old front) and throws the skater backwards at speed 10. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/vector.h"

void CBruce::HandleLeaningBail(SVECTOR s) {
	CVector v;
	CVector t;

	if (mInAir == 0) {
		if (mPhysicsState != 2) {
			v.vx = s.vx;
			v.vy = s.vy;
			v.vz = s.vz;

			if (Mips_CVecDotProd(&mUp, &v) < 2000 && mUp.vy >= -1999) {
				Bail(0);

				mAnimSpeed = 0x10000;
				Debug_RunAnim(56, 4860);

				m24C = 9;
				t = mFront;
				mFront = -mUp;
				mUp = t;
				RotMatrixFromFRUVectors();

				mVel = -mFront * 10;
				mVel.vy = 0;
			}
		}
	}
}
