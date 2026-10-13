/* DoWallRidePhysics__6CBruce (0x800376f4) */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. Wall ride: moves the skater by the
   frame-scaled velocity and kick, turns along the fall line, sweeps the movement against the
   world (snapping to the wall and re-orienting if it was hit), then probes along -mUp for the
   wall: on it, the skater is stuck to it and gravity projected into its plane is added; off it,
   the skater drops back into the air.
   Levers: TimeScale/1778 are plain `extern int` (one `la`/`lw` macro insn each, so the HIGH
   part isn't CSE'd into a saved reg); SetVector is a macro with block locals _x/_y/_z (as in the
   demo's MAIN.SYM blocks); `CheckDist` as a variable gives retail's `slt` with 116 in a register;
   the `Steep` int gives the slti/xori scc; the unused SquareRoot0 call goes through a void cast. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/colij.h"
#include "game/bruce.h"

#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}

extern int TimeScale;
extern int TimeScaleSquared;
extern CVector ZeroVector;
extern SPSXRegion PSXRegion[];

int Face_Air;
int Face_Skateable;
int Face_Visible;

short PingPong(int, int, int);
void ProjectCVectorToPlane(CVector *, SVECTOR);
void M3dUtils_NormaliseVector(CVector *);
void CVectorRotateToPlane(CVector *, SVECTOR *, CVector *);
void M3dColij_InitLineInfo(SLineInfo *);
int GetFaceFlags(SLineInfo *);

extern "C" {
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
	int Mips_FixMul__Fii(int, int);
	int M3dZone_LineToItemNotSCached__FP9SLineInfoi(SLineInfo *, int);
	SMemHandle FUN_8004d7ac(void *);
	void FUN_8002f1a0(CBruce *);
	void SFX_Grunt__FP6CBruce(CBruce *);
}

void CBruce::DoWallRidePhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector InplaneGravity;
	CVector u3, u4, u5; /* unused, likewise */
	SVECTOR u6;
	int Speed;
	int TurnAngle;
	SLineInfo line_info;
	int OnInvisible;
	CVector Gravity;

	Gravity.vx = 0;
	Gravity.vy = 9000;
	Gravity.vz = 0;
	OnInvisible = mPhysicsState == 2;

	if (mAnimFinished) {
		if (mAnim == 36)
			mFrame = PingPong(7, 9, 15);
		else if (mAnim == 35)
			mFrame = PingPong(7, 9, 15);
	}

	mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
	vMovement = mPos - mOldPos;
	/* result unused; called as void so v0 isn't reserved (retail's register choice) */
	((void (*)(int))FUN_8008b5dc)(Mips_CVecDotProd__FP7CVectorT0(&vMovement, &vMovement));
	TurnAngle = 0;

	if (!m720 && !mTapTurn) {
		AlignToFallLine(4);

		TurnAngle = mFallLineTurn;
	}

	Mips_FixMul__Fii(TurnAngle, 3072);
	RotateHeadingAndVelocity((TurnAngle * TimeScale) >> 8, !OnInvisible, 0);

	SetVector(&line_info.StartCoords, mOldPos.vx, mOldPos.vy, mOldPos.vz)
	    SetVector(&line_info.EndCoords, mPos.vx, mPos.vy, mPos.vz) vMovement = mPos - mOldPos;

	if (vMovement != ZeroVector) {
		Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&vMovement, &vMovement)) << 6;

		if (mInAir ? Speed < 0x64000 : Speed < 0x1e000) {
			if (Speed < 2048)
				Speed = 2048;

			line_info.EndCoords.vx =
			    mOldPos.vx + ((vMovement.vx * (mInAir ? 100 : 30) / Speed) << 12);
			line_info.EndCoords.vy =
			    mOldPos.vy + ((vMovement.vy * (mInAir ? 100 : 30) / Speed) << 12);
			line_info.EndCoords.vz =
			    mOldPos.vz + ((vMovement.vz * (mInAir ? 100 : 30) / Speed) << 12);
		}

		M3dColij_InitLineInfo(&line_info);
		M3dZone_LineToItemNotSCached__FP9SLineInfoi(&line_info, 1);
		GetFaceFlags(&line_info);

		if (line_info.pItem) {
			if (Face_Visible || !Face_Air) {
				mPos.vx = line_info.Position.vx + mUp.vx * 30;
				mPos.vy = line_info.Position.vy + mUp.vy * 30;
				mPos.vz = line_info.Position.vz + mUp.vz * 30;

				if (!OnInvisible) {
					mpWallItem = line_info.pItem;
					mWallHandle = FUN_8004d7ac(PSXRegion[line_info.pItem->Region].pPSX);
				}

				if (*(int *)&line_info.Normal == *(int *)&mNormal &&
				    line_info.Normal.vz == mNormal.vz) {
					mSameNormal++;
				} else {
					mSameNormal = 0;
					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
				}

				OrientToNormal();

				if (Face_Skateable) {
					StateTransition(PHYSICS_ON_GROUND, 5840);
					ProjectCVectorToPlane(&mFront, line_info.Normal);
					M3dUtils_NormaliseVector(&mFront);
					SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
					    gte_ldopv1(&mFront);
					gte_ldopv2(&mUp);
					gte_op12();
					gte_stlvnl(&mRight);
					RotMatrixFromFRUVectors();

					mSameNormal = 0;
					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
					    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz)
					        ProjectCVectorToPlane(&mVel, line_info.Normal);
				}
			}
		}
	}

	if (mMaxSpeed < FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6)
		m304 = 1;
	else
		m304 = 0;

	FRUVectorsFromRotMatrix();

	line_info.StartCoords.vx = mPos.vx + mUp.vx * 70;
	line_info.StartCoords.vy = mPos.vy + mUp.vy * 70;
	line_info.StartCoords.vz = mPos.vz + mUp.vz * 70;
	line_info.EndCoords.vx = line_info.StartCoords.vx - mUp.vx * 128;
	line_info.EndCoords.vy = line_info.StartCoords.vy - mUp.vy * 128;
	line_info.EndCoords.vz = line_info.StartCoords.vz - mUp.vz * 128;
	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached__FP9SLineInfoi(&line_info, 1);
	GetFaceFlags(&line_info);

	if (line_info.pItem) {
		int d = line_info.Distance;
		int CheckDist = 116;

		if (d <= CheckDist) {
			m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;
			mPos.vx = line_info.Position.vx + mUp.vx * 30;
			mPos.vy = line_info.Position.vy + mUp.vy * 30;
			mPos.vz = line_info.Position.vz + mUp.vz * 30;

			if (!OnInvisible) {
				mpWallItem = line_info.pItem;
				mWallHandle = FUN_8004d7ac(PSXRegion[line_info.pItem->Region].pPSX);
			}

			if (*(int *)&line_info.Normal == *(int *)&mNormal &&
			    line_info.Normal.vz == mNormal.vz) {
				mSameNormal++;
			} else {
				mSameNormal = 0;
				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
			}

			OrientToNormal();

			if (Face_Skateable) {
				StateTransition(PHYSICS_ON_GROUND, 5949);
				ProjectCVectorToPlane(&mFront, line_info.Normal);
				M3dUtils_NormaliseVector(&mFront);
				SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				    gte_ldopv1(&mFront);
				gte_ldopv2(&mUp);
				gte_op12();
				gte_stlvnl(&mRight);
				RotMatrixFromFRUVectors();

				mSameNormal = 0;
				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) ProjectCVectorToPlane(&mVel, line_info.Normal);
			} else {
				int Steep = line_info.Normal.vy >= 3278;

				if (Steep) {
					mPos = mOldPos;
					StateTransition(PHYSICS_IN_AIR, 5987);

					m740 = 1;
					mAcc += Gravity;
					ReflectHeadingVelocityInPlane(line_info.Normal, 200);
					SFX_Grunt__FP6CBruce(this);
				}
			}
		}

		InplaneGravity = Gravity;
		ProjectCVectorToPlane(&InplaneGravity, line_info.Normal);

		mAcc += InplaneGravity;
		mFallLine = InplaneGravity;
		mPolyNormal = line_info.Normal;
		CVectorRotateToPlane(&mVel, &line_info.Normal, &mVel);
	} else {
		StateTransition(PHYSICS_IN_AIR, 6040);
		FUN_8002f1a0(this);

		mTallyAngles = 0;
		mNum180Spins = 0;
		mDoingATrick = 0;
		m740 = 1;

		if (!mInAir) {
			mAnimSpeed = 0x10000;

			if (mManIsCrouched)
				Debug_RunAnim(26, 6071);
			else
				Debug_RunAnim(27, 6073);
		}

		mAcc += Gravity;
	}
}
