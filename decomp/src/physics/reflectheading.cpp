/* ReflectHeadingVelocityInPlane__6CBruceG7SVECTORi (0x80039f2c) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
#ifndef PC_PORT
	#include <inline_c.h>
#endif
}

#include "game/bruce.h"
#include "game/m3dasm.h"
#include "game/vector.h"

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

	dot = Mips_CVecDotProd(&mVel, &TmpCV);

	if (dot < 0) {
		mVel.vx -= Mips_FixMul(Normal.vx, dot);
		mVel.vy -= Mips_FixMul(Normal.vy, dot);
		mVel.vz -= Mips_FixMul(Normal.vz, dot);
		mVel.vx += Mips_FixMul(Normal.vx, 205);
		mVel.vy += Mips_FixMul(Normal.vy, 205);
		mVel.vz += Mips_FixMul(Normal.vz, 205);
	}

	{
		int _x = -Normal.vx;
		int _y = -Normal.vy;
		int _z = -Normal.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd(&mFront, &TmpCV);

	if (dot < 0) {
		int sin;
		int cos;
		short *pShort;
		SVECTOR RotAngle;

		sin = M3dMaths_SquareRoot0(0x1000000 - dot * dot);
		pShort = (short *)&SurfaceRotMatrix;
		cos = dot;
		dot = Mips_CVecDotProd(&mRight, &TmpCV);

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
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);

		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = (dot > 0 ? Angle - 1024 : 1024 - Angle) & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
		FRUVectorsFromRotMatrix();
		M3dUtils_NormaliseVector(&mFront);
		M3dUtils_NormaliseVector(&mUp);
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
		geom_cross(&mUp, &mRight, &mFront, 12);
#else
		gte_ldopv1(&mUp);
		gte_ldopv2(&mRight);
		gte_op12();
		gte_stlvnl(&mFront);
#endif
		RotMatrixFromFRUVectors();
	}
}
