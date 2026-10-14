/* RunAnim__6CSuperiiii (0x80043e0c) */
/* C++ (OB.CPP). Demo twin (same shape); locals from its MAIN.SYM. Starts an animation: clamps the
   anim index to the model's table, then the from/to frames (-1 = last), and sets the play direction.
   The direction byte is signed so its -1 shares the register of the "from == -1" test. */
#include "game/ob.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[] __attribute__((section(".data")));

void CSuper::RunAnim(int a, int from, int to, int to2) {
	mAnim = a;

	if (mAnim >= *PSXRegion[mRegion].pAnimFile)
		mAnim = 46;

	mNumFrames = ((unsigned char *)PSXRegion[mRegion].pAnimFile)[mAnim * 8 + 8];

	if (from == -1)
		from = mNumFrames - 1;

	if (to == -1)
		to = mNumFrames - 1;

	if (from < 0)
		from = 0;

	if (to < 0)
		to = 0;

	if (from >= mNumFrames)
		from = mNumFrames - 1;

	if (to >= mNumFrames)
		to = mNumFrames - 1;

	mAnimMode = 0;

	if (from >= to) {
		if (to < from)
			mAnimDir = -1;
		else
			mAnimDir = 0;
	} else
		mAnimDir = 1;

	mFrame = from;
	mTargetFrame = to;
	mTargetFrame2 = to2;
	mFrameFrac = 0;
	mAnimFinished = (short)from == (signed char)to;
}
