/* HandleBailPerpendicular__6CBruceG7SVECTOR (0x80031224) */
/* C++ (PHYSICS.CPP). Fuzzy demo twin (0.83; retail is trimmed), locals from its MAIN.SYM. When the
   skater runs into a wall (Normal) fast and nearly head-on, flips them, faces them back along
   their last movement, rebuilds the orientation against the wall normal and, if they were
   facing away, starts the perpendicular bail. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
#include "game/bruce.h"
#include "game/vector.h"

static inline void SetVector(VECTOR *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

void ProjectCVectorToPlane(CVector *, SVECTOR);

void CBruce::HandleBailPerpendicular(SVECTOR Normal) {
	CVector InplaneVel;
	int Speed;
	int Dot;

	InplaneVel = mVel;
	ProjectCVectorToPlane(&InplaneVel, Normal);

	Speed = SquareRoot0(Mips_CVecDotProd(&InplaneVel, &InplaneVel)) << 6;

	if (Speed <= 0x1e000)
		return;

	Dot = (Mips_CVecDotProd(&InplaneVel, &mFront) << 6) / (Speed >> 6);

	if (ABS(Dot) >= 1638)
		return;

	FlipToVelocity();

	{
		int RightDot = Mips_CVecDotProd(&InplaneVel, &mRight);

		Bail(0);

		mPerpendicularBails++;

		{
			VECTOR v;
			CVector vv;

			vv = mOldPos - m1B0;
			SetVector(&v, vv.vx, vv.vy, vv.vz);

			if (ABS(v.vx) + ABS(v.vy) + ABS(v.vz) > 0x8000)
				SetVector(&v, v.vx >> 4, v.vy >> 4, v.vz >> 4);

			VectorNormal(&v, &v);

			mFront.vx = -v.vx;
			mFront.vy = -v.vy;
			mFront.vz = -v.vz;
			mUp.vx = Normal.vx;
			mUp.vy = Normal.vy;
			mUp.vz = Normal.vz;
			geom_cross(&mFront, &mUp, &mRight, 12);
			geom_cross(&mUp, &mRight, &mFront, 12);
			RotMatrixFromFRUVectors();
		}

		if (mExtraFlags & 2)
			RightDot = -RightDot;

		if (RightDot <= 0) {
			mAnimSpeed = 0x10000;
			Debug_RunAnim(46, 1997);

			m24C = 5;
		}
	}
}
