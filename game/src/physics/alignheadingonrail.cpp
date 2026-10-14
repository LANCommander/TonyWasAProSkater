/* AlignHeadingOnRail__6CBruce (0x80033108) */
/* C++ (PHYSICS.CPP). Stands the skater upright on a rail: up is straight up, right = front x up
   (normalised), up = right x front, then rebuilds the rotation matrix. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void M3dUtils_NormaliseVector(CVector *);

void CBruce::AlignHeadingOnRail() {
	mUp.vx = 0;
	mUp.vy = -4096;
	mUp.vz = 0;
	geom_cross(&mFront, &mUp, &mRight, 12);
	M3dUtils_NormaliseVector(&mRight);
	geom_cross(&mRight, &mFront, &mUp, 12);
	RotMatrixFromFRUVectors();
}
