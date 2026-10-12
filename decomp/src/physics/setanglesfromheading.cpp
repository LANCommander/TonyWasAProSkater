/* SetAnglesFromHeading__6CBruce (0x8003a648) */
/* C++ (PHYSICS.CPP). Euler angles from the rotation matrix, with its up column negated for the
   conversion and restored afterwards. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void M3dUtils_AnglesFromMatrix(SVECTOR *, MATRIX *);

void CBruce::SetAnglesFromHeading() {
	SVECTOR Angles;

	mRotMatrix.m[0][1] = -mRotMatrix.m[0][1];
	mRotMatrix.m[1][1] = -mRotMatrix.m[1][1];
	mRotMatrix.m[2][1] = -mRotMatrix.m[2][1];
	M3dUtils_AnglesFromMatrix(&Angles, &mRotMatrix);

	mRotMatrix.m[0][1] = -mRotMatrix.m[0][1];
	mRotMatrix.m[1][1] = -mRotMatrix.m[1][1];
	mRotMatrix.m[2][1] = -mRotMatrix.m[2][1];
	mAngles.vx = Angles.vx;
	mAngles.vy = Angles.vy;
	mAngles.vz = Angles.vz;
}
