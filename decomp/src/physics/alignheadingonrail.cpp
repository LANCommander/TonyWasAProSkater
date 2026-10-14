/* AlignHeadingOnRail__6CBruce (0x80033108) */
/* C++ (PHYSICS.CPP). Stands the skater upright on a rail: up is straight up, right = front x up
   (normalised), up = right x front, then rebuilds the rotation matrix. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
#ifndef PC_PORT
	#include <inline_c.h>
#endif
}

#include "game/bruce.h"

struct Message;

void M3dUtils_NormaliseVector(CVector *);

void CBruce::AlignHeadingOnRail() {
	mUp.vx = 0;
	mUp.vy = -4096;
	mUp.vz = 0;
#ifdef PC_PORT
	geom_cross(&mFront, &mUp, &mRight, 12);
#else
	gte_ldopv1(&mFront);
	gte_ldopv2(&mUp);
	gte_op12();
	gte_stlvnl(&mRight);
#endif
	M3dUtils_NormaliseVector(&mRight);
#ifdef PC_PORT
	geom_cross(&mRight, &mFront, &mUp, 12);
#else
	gte_ldopv1(&mRight);
	gte_ldopv2(&mFront);
	gte_op12();
	gte_stlvnl(&mUp);
#endif
	RotMatrixFromFRUVectors();
}
