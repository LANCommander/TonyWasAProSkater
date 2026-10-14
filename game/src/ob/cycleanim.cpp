/* CycleAnim__6CSuperiSc (0x80043f24) */
/* C++ (OB.CPP). Starts animation `anim` looping (mode 1) in direction `dir` from frame 0, taking
   its frame count from the region's animation file. */
#include "game/ob.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

void CSuper::CycleAnim(int anim, signed char dir) {
	uchar num_frames;

	mAnim = anim;
	num_frames = ((uchar *)PSXRegion[mRegion].pAnimFile)[mAnim * 8 + 8];
	mAnimMode = 1;
	mAnimDir = dir;
	mFrame = 0;
	mFrameFrac = 0;
	mAnimFinished = 0;
	mNumFrames = num_frames;
}
