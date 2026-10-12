/* UprightSideways__6CBruce (0x8003a498) */
/* C++ (PHYSICS.CPP). Demo twin; locals from its MAIN.SYM. Rolls the skater a notch back toward
   upright: the front x world-up cross product (GTE op) dotted with the up vector picks a roll
   direction, applied by multiplying in a Z rotation. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/bruce.h"

extern "C" int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);
extern "C" void FUN_80093130(MATRIX *, MATRIX *);
extern "C" MATRIX *FUN_80093240(SVECTOR *, MATRIX *);

extern CVector WorldUp;

void CBruce::UprightSideways() {
	CVector TmpCV;
	int dot;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	VECTOR v;

	gte_ldopv1(&mFront);
	gte_ldopv2(&WorldUp);
	gte_op12();
	gte_stlvnl(&v);

	{
		int _x = v.vx;
		int _y = v.vy;
		int _z = v.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd__FP7CVectorT0(&mUp, &TmpCV);

	if (dot >= 42) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 11;
		FUN_80093240(&RotAngle, &SurfaceRotMatrix);
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);
	} else if (dot < -41) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 4085;
		FUN_80093240(&RotAngle, &SurfaceRotMatrix);
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);
	} else {
		Mips_CVecDotProd__FP7CVectorT0(&mUp, &WorldUp);
	}

	FRUVectorsFromRotMatrix();
}
