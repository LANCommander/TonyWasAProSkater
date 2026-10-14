/* UprightSideways__6CBruce (0x8003a498) */
/* C++ (PHYSICS.CPP). Demo twin; locals from its MAIN.SYM. Rolls the skater a notch back toward
   upright: the front x world-up cross product (GTE op) dotted with the up vector picks a roll
   direction, applied by multiplying in a Z rotation. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
#ifndef PC_PORT
	#include <inline_c.h>
#endif
}

#include "game/bruce.h"
#include "game/vector.h"

extern CVector WorldUp;

void CBruce::UprightSideways() {
	CVector TmpCV;
	int dot;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	VECTOR v;

#ifdef PC_PORT
	geom_cross(&mFront, &WorldUp, &v, 12);
#else
	gte_ldopv1(&mFront);
	gte_ldopv2(&WorldUp);
	gte_op12();
	gte_stlvnl(&v);
#endif

	{
		int _x = v.vx;
		int _y = v.vy;
		int _z = v.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd(&mUp, &TmpCV);

	if (dot >= 42) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 11;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
	} else if (dot < -41) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 4085;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
	} else {
		Mips_CVecDotProd(&mUp, &WorldUp);
	}

	FRUVectorsFromRotMatrix();
}
