/* UpdateShadow__5CBody (0x80043b50) */
/* C++ (OB.CPP). Exact demo twin; written from its MAIN.SYM (up/trans in the flag block). The
   shadow quad comes from CBit's pool allocator (operator new, null-checked). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/ob.h"
#include "game/bit.h"

extern int UseCacheForBits;

void CBody::UpdateShadow() {
	if (mCBodyFlags & 8) {
		SVECTOR up;
		int trans;

		if (!mpShadow) {
			UseCacheForBits = 0;
			CQuadBit *p = new CQuadBit;

			mpShadow = p;
			UseCacheForBits = 1;
			p->SetTexture("Shadow", 0);
			mpShadow->FlagOn(4);
			mpShadow->SetSemiTransparent();

			mpShadow->mFrigDeltaZ = 32;
			mpShadow->mProtected = 1;
		}

		if (mShadowNormal.vy < -128) {
			up.vx = mShadowNormal.vx;
			up.vy = mShadowNormal.vy;
			up.vz = mShadowNormal.vz;
			mpShadow->OrientUsing(mShadowPos, up, mShadowScale, mShadowScale, 0);
		}

		trans = ((mShadowThreshold - mShadowDist) << 7) / mShadowThreshold;

		if (trans < 0)
			trans = 0;

		trans /= 6;
		mpShadow->SetTransparency(trans);
	} else {
		KillShadow();
	}
}
