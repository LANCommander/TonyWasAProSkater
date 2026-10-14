/* CM_Normal__7CCamera (0x8007c240) */
/* C++ (CAMERA.CPP). Fuzzy demo twin (opcode-identical but for one instruction); locals from its
   MAIN.SYM. THPS1's CCamera members sit 0x2c past the demo's (its CBody is bigger). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/camera.h"
#include "game/bruce.h"

CQuat Quat_FromYRot(short);
void MToQ(const MATRIX &, CQuat &);

int Mips_CVecDotProd(CVector *, CVector *);
void Utils_CalcAim(CSVector &, const CVector &, const CVector &);
void Utils_GetVecFromMagDir(CVector &, int, const CSVector &);

extern CBruce *GSkater;
extern MATRIX tm;

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

void CCamera::CM_Normal() {
	CVector vUp;
	CVector vRight;
	CSVector ang1;
	int dot_up;
	int dot_front;
	CVector temp_vel = mTripodPos - mOldTripodPos;

	vUp = ((CBruce *)mTripod)->mUp;
	CVector Down(0, 4096, 0);
	CVector Zero(0, 0, 0);
	CVector temp_front;

	dot_up = ABS(Mips_CVecDotProd(&Down, &vUp));
	Utils_CalcAim(ang1, Zero, temp_vel);
	Utils_GetVecFromMagDir(temp_front, 4096, ang1);

	dot_front = ABS(Mips_CVecDotProd(&temp_front, &vUp));

	in_pool_timer++;

	if (dot_up < 3572 &&
	    (((CBruce *)mTripod)->mPhysicsState == 2 || ((CBruce *)mTripod)->mPhysicsState == 8)) {
		if (!big_air && ((CBruce *)mTripod)->m740)
			((CBruce *)mTripod)->mBigAirs++;

		if (++turn_delay_timer >= 4)
			vFront = CVector(0, 0x1000000, 0);
		else
			Utils_GetVecFromMagDir(vFront, 4096, ang1);

		big_air = 1;
		in_pool_timer = 0;
	} else if ((dot_front >> 12) > 2000 || temp_vel.Length() < 5) {
		turn_delay_timer = 0;

		if (dot_up < 3572 && big_air == 1)
			in_pool_timer = 0;

		big_air = 0;
	} else {
		turn_delay_timer = 0;
		Utils_GetVecFromMagDir(vFront, 4096, ang1);

		if (dot_up < 3572 && big_air == 1)
			in_pool_timer = 0;

		big_air = 0;
	}

	mOldVFront = mVFront;

	if (mTripodPos == mOldTripodPos)
		mVFront = mOldVFront;
	else
		mVFront = (mOldVFront + vFront) >> 1;

	if (mTimer < 12) {
		mVFront = (-GSkater->mFront) << 12;
		mOldVFront = mVFront;
		vFront = mOldVFront;
	}

	vFront = mVFront;

	CVector U;
	CVector V;
	CVector N;

	N = vFront >> 12;
	V.vx = -vUp.vx;
	V.vy = -vUp.vy;
	V.vz = -vUp.vz;
	geom_cross(&V, &N, &U, 0);
	U.vx >>= 12;
	U.vy >>= 12;
	U.vz >>= 12;
	geom_cross(&U, &V, &N, 0);
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
}
