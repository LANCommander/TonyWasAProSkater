/* FRUVectorsFromRotMatrix__6CBruce (0x8003a5a4) */
/* C++ (PHYSICS.CPP). Front, right and up vectors from the rotation matrix's columns 2, 0 and 1. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::FRUVectorsFromRotMatrix() {
	mFront.vx = mRotMatrix.m[0][2];
	mFront.vy = mRotMatrix.m[1][2];
	mFront.vz = mRotMatrix.m[2][2];
	mRight.vx = mRotMatrix.m[0][0];
	mRight.vy = mRotMatrix.m[1][0];
	mRight.vz = mRotMatrix.m[2][0];
	mUp.vx = mRotMatrix.m[0][1];
	mUp.vy = mRotMatrix.m[1][1];
	mUp.vz = mRotMatrix.m[2][1];
}
