/* MoveTo__9CPlatformR7CVector (0x8003ebdc) */
/* C++ (PLATFORM.CPP). Sets a new target position; flags the platform as moving if it changed. */
#include "game/platform.h"

void CPlatform::MoveTo(CVector &pos) {
	if (mTargetPos != pos) {
		mTargetPos = pos;
		mSpecificFlags |= 1;
	}
}
