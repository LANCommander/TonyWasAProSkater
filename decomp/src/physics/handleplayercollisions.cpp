/* HandlePlayerCollisions__6CBruce (0x8003b2f0) */
/* C++ (PHYSICS.CPP). Two-player skater-vs-skater collision: when both skaters are on the ground and
   within 60 units, compares each one's velocity along the line between them; the one moving into
   the other (or the faster of two closing skaters) knocks the other down, else bails itself.
   The other skater pointer lives at 0x1d4 (bruce.h has only the pad there). */
#include "game/bruce.h"
#include "game/vector.h"

void CBruce::HandlePlayerCollisions() {
	int s0, s1, v;
	CVector *s3;

	if (mInAir == 0) {
		if (mpOtherSkater->mInAir == 0) {
			CVector d = mpOtherSkater->mPos - mPos;

			if (d.Length() < 60) {
				CVector c = mpOtherSkater->mPos - mOldPos;

				d = c;
				d.Normalize();

				s3 = &mVel;
				s1 = Mips_CVecDotProd(s3, &d);
				v = Mips_CVecDotProd(&mpOtherSkater->mVel, &d);

				if (s1 > 0) {
					if (v > 0) {
						mpOtherSkater->Bail(0);
					} else {
						s0 = mpOtherSkater->mVel.Length();

						if (s3->Length() >= s0)
							mpOtherSkater->Bail(0);
						else
							Bail(0);
					}
				} else if (v <= 0)
					Bail(0);
			}
		}
	}
}
