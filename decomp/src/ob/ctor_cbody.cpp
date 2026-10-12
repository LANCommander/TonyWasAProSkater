/* __5CBody (0x800437d4) */
#include "game/ob.h"

CBody::CBody() : mFric(1, 1, 1), mAngFric(1, 1, 1) {
	mPushVal = 10;
	m98 = 0;
	mNode = 0xffff;
	mShadowScale = 50;
	mShadowThreshold = 300;
	mCBodyFlags |= 18;
	mRMinor = 50;
}
