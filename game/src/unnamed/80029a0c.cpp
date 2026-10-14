/* __11CMotionBlurRC7CVectorR7CVectorPciii (0x80029a0c) */
/* C++ (BIT.CPP). A semi-transparent animated flat bit moving with a fixed velocity, shrinking and
   fading as it goes. */
#include "game/bit.h"

CMotionBlur::CMotionBlur(const CVector &pos, CVector &vel, char *pAnimName, int scale,
                         int scaleDecay, int transDecay) {
	mPos = pos;
	mVel = vel;
	SetAnim(pAnimName);
	SetSemiTransparent();

	mScale = scale;
	mScaleDecay = scaleDecay;
	mTransDecay = transDecay;
}
