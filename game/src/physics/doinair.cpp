/* DoInAirPhysics__6CBruce (0x8003609c) */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. In the air: moves the skater by the
   frame-scaled velocity and kick, applies the surface spin, turns by the spin input, keeps the
   skater upright, then sweeps the movement against the world. Hitting a wall that is steep
   enough at a shallow angle starts a wall ride (or bails when going too slowly for the wall);
   otherwise a wall knocks the skater back down and a floor lands them (grind / manual / bail
   checks). No hit: the gravity is added to the kick.
   Levers: unused CVector locals keep retail's frame slots; WallRight is declared just before its
   GTE use (so the by-value SVECTOR argument temps share its slot, as in retail); the unused
   SquareRoot0 call goes through a void cast and the libgte calls return MATRIX * / VECTOR *
   (v0/v1 choice); the m368 trick-limit test is if / else-if with a Trick index local; the
   landing sound is a switch with a call per arm; m200 = 15 is stored twice around the mPos
   update (the demo's MoveOriginTo call sat between the two stores). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/colij.h"
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/vector.h"
#include "game/m3dasm.h"
#include "game/psyq.h"

#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}

extern int XFrames;
extern int TimeScale;
extern int TimeScaleSquared;
extern int GLevel;
extern int BulletCollisionCheck;
extern CVector ZeroVector;
extern CVector WorldUp;
extern STrick Group[];

int Face_Air;
int Face_Skateable;
int Face_WallRide;
int Face_Visible;
int Face_Terrain;

void Platform_HandleBounce();
void ProjectCVectorToPlane(CVector *, SVECTOR);
void M3dUtils_NormaliseVector(CVector *);
void M3dColij_InitLineInfo(SLineInfo *);
int GetFaceFlags(SLineInfo *);
int SFX_PlayX(unsigned int, short, int);
int Rnd(int);

void CBruce::DoInAirPhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector u3, u4;
	int Speed;
	int TurnAngle;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	SLineInfo line_info;
	CVector Gravity;

	Gravity = mGravity;
	mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
	Platform_HandleBounce();

	if (mPitchAngle) {
		VECTOR zero, v, v2, v3;

		zero.vz = 0;
		zero.vy = 0;
		zero.vx = 0;
		TransMatrix(&mRotMatrix, &zero);
		TransMatrix(&SurfaceRotMatrix, &zero);

		v.vx = 0;
		v.vy = 0x46000;
		v.vz = 0;
		ApplyMatrixLV(&mRotMatrix, &v, &v2);

		RotAngle.vz = 0;
		RotAngle.vy = 0;
		RotAngle.vx = mPitchAngle & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
		ApplyMatrixLV(&mRotMatrix, &v, &v3);

		mPos.vx = mPos.vx + v2.vx - v3.vx;
		mPos.vy = mPos.vy + v2.vy - v3.vy;
		mPos.vz = mPos.vz + v2.vz - v3.vz;
	}

	vMovement = mPos - mOldPos;
	((void (*)(int))M3dMaths_SquareRoot0)(Mips_CVecDotProd(&vMovement, &vMovement));
	TurnAngle = mLeanAngle >> 12;
	Mips_FixMul(TurnAngle, 3072);

	if (TurnAngle && m814 == ZeroVector) {
		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = TurnAngle & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);

		mTallyAngles += TurnAngle;

		if (mTallyAngles >= 2048) {
			mTallyAngles -= 2048;

			if (!mNoSpinScore)
				mNum180Spins++;
		}

		if (mTallyAngles <= -2048) {
			mTallyAngles += 2048;

			if (!mNoSpinScore)
				mNum180Spins++;
		}
	}

	if (mPhysicsState != 2)
		UprightSideways();

	SetVector(&line_info.StartCoords, mOldPos.vx, mOldPos.vy, mOldPos.vz)
	    SetVector(&line_info.EndCoords, mPos.vx, mPos.vy, mPos.vz) vMovement = mPos - mOldPos;
	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&vMovement, &vMovement)) << 6;

	if (Speed < 0x2e000) {
		if (Speed < 2048)
			Speed = 2048;

		line_info.EndCoords.vx = mOldPos.vx + ((vMovement.vx * 46 / Speed) << 12);
		line_info.EndCoords.vy = mOldPos.vy + ((vMovement.vy * 46 / Speed) << 12);
		line_info.EndCoords.vz = mOldPos.vz + ((vMovement.vz * 46 / Speed) << 12);
	}

	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached(&line_info, 1);
	GetFaceFlags(&line_info);

	if (Face_Air && !Face_Visible && mPhysicsState != 3) {
		BulletCollisionCheck = 1;
		M3dColij_InitLineInfo(&line_info);
		M3dZone_LineToItemNotSCached(&line_info, 1);
		GetFaceFlags(&line_info);

		BulletCollisionCheck = 0;
	}

	if (line_info.pItem) {
		if ((!Face_Skateable || (Face_Air && Face_Visible)) && Face_WallRide && !mInAir &&
		    (mpPad->Grind.Pressed || mpPad->Grind.ReleasedTime < 20)) {
			CVector UnitVel;
			CVector FaceNormal;
			int VelNormal;
			int Steep = line_info.Normal.vy >= 3278;

			UnitVel = mVel;
			UnitVel.vy = 0;
			M3dUtils_NormaliseVector(&UnitVel);

			FaceNormal.vx = line_info.Normal.vx;
			FaceNormal.vy = 0;
			FaceNormal.vz = line_info.Normal.vz;
			M3dUtils_NormaliseVector(&FaceNormal);

			VelNormal = Mips_CVecDotProd(&FaceNormal, &UnitVel);

			if (!Steep && ((VelNormal ^ (VelNormal >> 31)) - (VelNormal >> 31)) < 3547 &&
			    XFrames - m208 >= 41) {
				int Limit = 10000;
				int Trick = m368 & 255;

				if (m368 & 0x4000)
					Limit = mNumFrames - Group[Trick].MinFrame;
				else if (m368 & 0x2000)
					Limit = mNumFrames - Group[Trick].MinFrame;

				if (mDoingATrick && !m360 && mFrame < Limit) {
					Bail(0);

					mTrickBails++;
				} else
					m360 = 1;

				if (!mInAir) {
					FrigDarkenObject();
					StateTransition(PHYSICS_IN_WALLRIDE, 5197);

					m208 = XFrames;
					mPos.vx = line_info.Position.vx + line_info.Normal.vx * 30;
					mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
					mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;
					ProjectCVectorToPlane(&mFront, line_info.Normal);
					M3dUtils_NormaliseVector(&mFront);
					SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
					    geom_cross(&mFront, &mUp, &mRight, 12);
					RotMatrixFromFRUVectors();
					FlipToVelocity();

					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
					    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz) mSameNormal = 0;
					ProjectCVectorToPlane(&mVel, line_info.Normal);
					CVector WallRight;

					geom_cross(&WorldUp, &mUp, &WallRight, 12);
					RotMatrixFromFRUVectors();

					mAnimSpeed = 0x10000;

					if (mExtraFlags & 2) {
						if (Mips_CVecDotProd(&WallRight, &mVel) >= 0)
							Debug_RunAnim(35, 5238);
						else
							Debug_RunAnim(36, 5242);
					} else {
						if (Mips_CVecDotProd(&WallRight, &mVel) >= 0)
							Debug_RunAnim(36, 5249);
						else
							Debug_RunAnim(35, 5253);
					}
				} else {
					mPos = mOldPos;
				}
			} else {
				mPos = mOldPos;
				printf("...............New Bail..............\n");
				Bail(0);
			}
		} else if (!Face_Skateable && (!Face_Air || mPhysicsState != 3)) {
			switch (Face_Terrain) {
				case 7:
					SFX_PlayX(53, 0x2000, 0);
					break;
				case 9:
					if (GLevel == 8)
						SFX_PlayX(137, 0x2000, 0);
					else
						SFX_PlayX(40, 0x2000, 0);

					break;
			}

			mNormal.vx = 0;
			mNormal.vy = -4096;
			mNormal.vz = 0;
			SetVector(&mOldNormal, 0, -4096, 0) mSameNormal = 0;
			OrientToNormal();

			m200 = 15;
			mPos.vx = line_info.Position.vx + line_info.Normal.vx;
			mPos.vy = line_info.Position.vy + line_info.Normal.vy;
			mPos.vz = line_info.Position.vz + line_info.Normal.vz;
			m200 = 15;
			StateTransition(PHYSICS_IN_AIR, 5363);

			m740 = 1;
			ReflectHeadingVelocityInPlane(line_info.Normal, 0);

			mVel.vx += line_info.Normal.vx / 4;
			mVel.vy += line_info.Normal.vy / 4;
			mVel.vz += line_info.Normal.vz / 4;

			if (!Rnd(3)) {
				mVel.vx += Rnd(16384) - 8192;
				mVel.vz += Rnd(16384) - 8192;
			}
		} else {
			mPos.vx = line_info.Position.vx + line_info.Normal.vx * 30;
			mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
			mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;

			if (!mInAir && Face_Air) {
				if (m764 && (!m1F8 || XFrames - m1F8 < 5 || Face_Visible)) {
					StateTransition(PHYSICS_ON_INVISIBLE, 5428);
				} else {
					StateTransition(PHYSICS_IN_AIR, 5435);

					if (mpPad->Up.Pressed || mAY < -40) {
						SVECTOR N;
						int Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

						N = line_info.Normal;
						mVel.vx -= Mips_FixMul(N.vx, Spd / 4);
						mVel.vz -= Mips_FixMul(N.vz, Spd / 4);
					}

					m768 = 0;

					if (!m740 || !mDoingATrick) {
						Trick_TakeOff();

						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 5457);

						m740 = 1;
					}
				}
			} else if (mPhysicsState != 3 || XFrames - m2FC >= 31) {
				StateTransition(PHYSICS_ON_GROUND, 5487);

				mTerrain = Face_Terrain;

				if (!mInAir) {
					HandleBailPerpendicular(line_info.Normal);
					HandleLeaningBail(line_info.Normal);
					HandleBailOnLanding(5496);
				}
			}

			m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;

			if (mPhysicsState != 1) {
				ProjectCVectorToPlane(&mVel, line_info.Normal);

				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) mSameNormal = 0;
				OrientToNormal();
			}
		}
	} else {
		mAcc += Gravity;
	}
}
