/* ReflectHeadingVelocityInPlane__6CBruceG7SVECTORi (0x80039f2c) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/bruce.h"

extern "C" int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);
extern "C" int Mips_FixMul__Fii(int, int);
extern "C" int FUN_8008b5dc(int);
extern "C" void FUN_80093130(MATRIX *, MATRIX *);
extern "C" void FUN_80093240(SVECTOR *, MATRIX *);
void M3dUtils_NormaliseVector(CVector *);

void CBruce::ReflectHeadingVelocityInPlane(SVECTOR Normal, int Angle) {
	CVector TmpCV;
	int dot;
	MATRIX SurfaceRotMatrix;
	CVector OldVel;

	if (!Angle)
		Angle = 25;

	OldVel = mVel;

	{
		int _x = Normal.vx;
		int _y = Normal.vy;
		int _z = Normal.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd__FP7CVectorT0(&mVel, &TmpCV);

	if (dot < 0) {
		mVel.vx -= Mips_FixMul__Fii(Normal.vx, dot);
		mVel.vy -= Mips_FixMul__Fii(Normal.vy, dot);
		mVel.vz -= Mips_FixMul__Fii(Normal.vz, dot);
		mVel.vx += Mips_FixMul__Fii(Normal.vx, 205);
		mVel.vy += Mips_FixMul__Fii(Normal.vy, 205);
		mVel.vz += Mips_FixMul__Fii(Normal.vz, 205);
	}

	{
		int _x = -Normal.vx;
		int _y = -Normal.vy;
		int _z = -Normal.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd__FP7CVectorT0(&mFront, &TmpCV);

	if (dot < 0) {
		int sin;
		int cos;
		short *pShort;
		SVECTOR RotAngle;

		sin = FUN_8008b5dc(0x1000000 - dot * dot);
		pShort = (short *)&SurfaceRotMatrix;
		cos = dot;
		dot = Mips_CVecDotProd__FP7CVectorT0(&mRight, &TmpCV);

		if (dot > 0)
			sin = -sin;

		pShort[0] = cos;
		pShort[1] = 0;
		pShort[2] = -sin;
		pShort[3] = 0;
		pShort[4] = 4096;
		pShort[5] = 0;
		pShort[6] = sin;
		pShort[7] = 0;
		pShort[8] = cos;
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);

		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = (dot > 0 ? Angle - 1024 : 1024 - Angle) & 4095;
		FUN_80093240(&RotAngle, &SurfaceRotMatrix);
		FUN_80093130(&mRotMatrix, &SurfaceRotMatrix);
		FRUVectorsFromRotMatrix();
		M3dUtils_NormaliseVector(&mFront);
		M3dUtils_NormaliseVector(&mUp);
		gte_ldopv1(&mFront);
		gte_ldopv2(&mUp);
		gte_op12();
		gte_stlvnl(&mRight);
		M3dUtils_NormaliseVector(&mRight);
		gte_ldopv1(&mUp);
		gte_ldopv2(&mRight);
		gte_op12();
		gte_stlvnl(&mFront);
		RotMatrixFromFRUVectors();
	}
}
