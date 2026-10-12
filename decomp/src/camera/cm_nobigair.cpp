/* CM_NoBigAir__7CCamera (0x8007cd78) */
/* C++ (CAMERA.CPP). Demo twin (0.92 by opcode; THPS1 always uses the tripod's up vector); locals from
   its MAIN.SYM. Camera mode for normal skating: smooths the travel direction into vFront, builds
   a look basis from it and the skater's up vector, and aims mDesiredO along it (plus extra_YRot).
   Down, Zero and U/V/N are declared where retail's frame shows them starting. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/camera.h"
#include "game/bruce.h"

extern MATRIX tm;

void MToQ(const MATRIX &, CQuat &);
extern "C" int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);
void Utils_CalcAim(CSVector &, const CVector &, const CVector &);
void Utils_GetVecFromMagDir(CVector &, int, const CSVector &);
CQuat Quat_FromYRot(short);

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

void CCamera::CM_NoBigAir() {
	CVector vUp;
	CVector vRight;
	CSVector ang1;
	int dot_front;
	CVector temp_vel = mTripodPos - mOldTripodPos;

	vUp = ((CBruce *)mTripod)->mUp;
	CVector Down(0, 4096, 0);
	CVector Zero(0, 0, 0);
	CVector temp_front;

	ABS(Mips_CVecDotProd__FP7CVectorT0(&Down, &vUp));
	Utils_CalcAim(ang1, Zero, temp_vel);
	Utils_GetVecFromMagDir(temp_front, 4096, ang1);

	dot_front = ABS(Mips_CVecDotProd__FP7CVectorT0(&temp_front, &vUp)) >> 12;

	if (dot_front > 2000 || temp_vel.Length() < 5) {
		big_air = 0;
	} else {
		big_air = 0;
		Utils_GetVecFromMagDir(vFront, 4096, ang1);
	}

	mOldVFront = mVFront;

	if (mTripodPos == mOldTripodPos)
		mVFront = mOldVFront;
	else
		mVFront = (mOldVFront + vFront) >> 1;

	vFront = mVFront;
	CVector U;
	CVector V;
	CVector N;

	N = vFront >> 12;
	V.vx = -vUp.vx;
	V.vy = -vUp.vy;
	V.vz = -vUp.vz;
	gte_ldopv1(&V);
	gte_ldopv2(&N);
	gte_op0();
	gte_stlvnl(&U);
	U.vx >>= 12;
	U.vy >>= 12;
	U.vz >>= 12;
	gte_ldopv1(&U);
	gte_ldopv2(&V);
	gte_op0();
	gte_stlvnl(&N);
	N.vx >>= 12;
	N.vy >>= 12;
	N.vz >>= 12;
	tm.m[0][0] = U.vx;
	tm.m[1][0] = U.vy;
	tm.m[2][0] = U.vz;
	tm.m[0][1] = V.vx;
	tm.m[1][1] = V.vy;
	tm.m[2][1] = V.vz;
	tm.m[0][2] = N.vx;
	tm.m[1][2] = N.vy;
	tm.m[2][2] = N.vz;
	MToQ(tm, mDesiredO);

	mDesiredO = mDesiredO * Quat_FromYRot(extra_YRot);
	big_air = 0;
}
