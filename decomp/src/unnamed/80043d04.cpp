/* __6CSuper (0x80043d04) */
/* C++: constructor of a CBody-derived class (vtable 0x800a0574). */
#include "game/ob.h"

CSuper::CSuper() {
	mExtraFlags = 0;
	mAnimSpeed = 0x10000;
	mNumFrames = 1;
	mAnimFinished = 1;
	mFlags |= 2;
}
