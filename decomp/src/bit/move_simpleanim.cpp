/* Move__11CSimpleAnim (0x80029974) */
/* C++ (BIT.CPP). Steps the animation. A one-shot anim dies on reaching mDieFrame; a looping one
   wraps to frame 0, or holds the last frame if mDieFrame is -2. */
#include "game/bit.h"

void CSimpleAnim::Move() {
	IncFrame();

	if (mDie) {
		if (mFrame >= mDieFrame)
			Die();
	} else if (mFrame >= mNumFrames) {
		if (mDieFrame == -2)
			SetFrame(mNumFrames - 1);
		else
			SetFrame(0);
	}
}
