/* AlignHeadingOnRail__6CBruce (0x80033108) */
/* C++ (PHYSICS.CPP). Stands the skater upright on a rail: up is straight up, right = front x up
   (normalised), up = right x front, then rebuilds the rotation matrix. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/bruce.h"

struct Message;

void M3dUtils_NormaliseVector(CVector *);

void CBruce::AlignHeadingOnRail() {
	mUp.vx = 0;
	mUp.vy = -4096;
	mUp.vz = 0;
	gte_ldopv1(&mFront);
	gte_ldopv2(&mUp);
	gte_op12();
	gte_stlvnl(&mRight);
	M3dUtils_NormaliseVector(&mRight);
	gte_ldopv1(&mRight);
	gte_ldopv2(&mFront);
	gte_op12();
	gte_stlvnl(&mUp);
	RotMatrixFromFRUVectors();
}
