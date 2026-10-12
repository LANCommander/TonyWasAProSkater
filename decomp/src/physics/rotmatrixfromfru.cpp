/* RotMatrixFromFRUVectors__6CBruce (0x8003a5fc) */
/* C++ (PHYSICS.CPP). The rotation matrix's columns 0, 1 and 2 from the right, up and front vectors. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::RotMatrixFromFRUVectors() {
	mRotMatrix.m[0][0] = mRight.vx;
	mRotMatrix.m[1][0] = mRight.vy;
	mRotMatrix.m[2][0] = mRight.vz;
	mRotMatrix.m[0][1] = mUp.vx;
	mRotMatrix.m[1][1] = mUp.vy;
	mRotMatrix.m[2][1] = mUp.vz;
	mRotMatrix.m[0][2] = mFront.vx;
	mRotMatrix.m[1][2] = mFront.vy;
	mRotMatrix.m[2][2] = mFront.vz;
}
