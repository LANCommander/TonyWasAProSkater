/* DoOnGroundPhysics__6CBruce (0x800347a8) size=5664 flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. One function: functions.tsv splits it into
   DoOnGroundPhysics__6CBruce (3092) + FUN_800353bc (4) + FUN_800353c0 (2568) at the OrientToNormal call, but
   the only prologue/epilogue pair spans all 5664 bytes and nothing calls 0x800353c0.
   On the ground: terrain sparks and pad rumble (rails/metal/etc. by mTerrain), moves the skater by the
   frame-scaled velocity and kick, turns along the fall line, sweeps the movement against the world
   (bouncing off walls, stepping up kerbs, sliding along), pushes out of side walls, then probes
   down along -mUp for the floor: on it, snaps to it, handles the bail / grind / wallride hand-offs
   and adds gravity projected into the floor plane; off it, skates off the ground. Finally the side
   velocity is removed and the forwards velocity damped (auto-kick, top speed, terrain drag).
   Levers: the mPos update is wrapped in do{}while(0) (its refs count double, so &mAcc wins fp
   over OnInvisible, which is spilled as in retail); CVector Old is declared mid-function at
   function scope, after the early temps, so it takes sp+344 and later block locals sit above it;
   -Normal.vy > ABS(Normal.vx) loads vy first; the rumble switch lists the empty cases 0,1,2,4
   (jump table from 0); CSpark is 92 bytes (one more int than the ctor TU declares).
   Residue (3 words): &mOldPos gets s1 where retail has s2. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
#include "game/colij.h"
#include "game/bruce.h"
#include "game/bit.h"
#include "game/career.h"

#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}

extern int GGame;
extern int GHorsePlayer;
extern SCareer GCareer;
extern int BitCount;
extern int UseCacheForBits;
extern int XFrames;
extern int TimeScale;
extern int TimeScaleSquared;
extern CVector ZeroVector;
extern SPSXRegion PSXRegion[];

int Face_Air;
int Face_Skateable;
int Face_WallRide;
int Face_Visible;
int Face_Terrain;

void Platform_HandleBounce();
void Bit_SetSparkTrajectory(const CSVector &);
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
	void Bit_SetSparkTrajectoryCone__FRC8CSVector(CSVector *);
	void Bit_SetSparkSize__FUi(int);
	void Bit_SetSparkRGB__FUcUcUc(int, int, int);
	void Bit_SetSparkFadeRGB__FUcUcUc(int, int, int);
	int Mem_GetFree__Fc(int);
	int FUN_800343f0(CBruce *, int);
	void FUN_8003434c(CBruce *);
	void FUN_8002f1a0(CBruce *);
	void SFX_Grunt__FP6CBruce(CBruce *);
}

void CBruce::DoOnGroundPhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector InplaneGravity;
	CVector u3, u4, u5;
	SVECTOR u6;
	int Speed;
	int TurnAngle;
	SLineInfo line_info;
	int OnInvisible;
	int AutoKickEnabled;
	CVector Gravity;

	OnInvisible = mPhysicsState == 2;

	if (GGame != 7)
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex];
	else
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex ^ GHorsePlayer];

	Gravity.vx = 0;
	Gravity.vy = 6500;
	Gravity.vz = 0;

	if (!mPhysicsState)
		m724 = 0;

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

	if ((mTerrain == 5 || mTerrain == 6 || mTerrain == 3) && BitCount < 200) {
		SetAnglesFromHeading();
		Bit_SetSparkTrajectory(mAngles);
		CSVector Cone;

		Cone.Set(512, 4096, 0);
		Bit_SetSparkTrajectoryCone__FRC8CSVector(&Cone);
		Bit_SetSparkSize__FUi(1);

		if (Speed > 0x10000) {
			int i;

			switch (mTerrain) {
				case 5:
					Bit_SetSparkRGB__FUcUcUc(2, 160, 32);
					break;
				case 6:
					Bit_SetSparkRGB__FUcUcUc(160, 160, 160);
					break;
				case 3:
					Bit_SetSparkRGB__FUcUcUc(128, 64, 32);
					break;
			}

			Bit_SetSparkFadeRGB__FUcUcUc(0, 0, 0);

			UseCacheForBits = 0;

			for (i = 0; i < 6; i++) {
				if (Mem_GetFree__Fc(1) > 10000) {
					CSpark *p = new CSpark(mPos, 20, Speed >> 16, 20);

					p->mVel += mVel;
				}
			}

			UseCacheForBits = 1;
		}
	}

	if (Speed > 0x10000) {
		Speed >>= 12;

		if (Speed > 64)
			Speed = 64;

		switch (mTerrain) {
			case 0:
			case 1:
			case 2:
			case 4:
				break;
			case 5:
				Vibrate(8, 1, 9600 / Speed);
				break;
			case 6:
				Vibrate(8, 1, 4480 / Speed);
				break;
			case 3:
				Vibrate(8, 1, 16320 / Speed);
				break;
		}
	}

	do {
		mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
	} while (0);

	Platform_HandleBounce();

	if (!OnInvisible && !mInAir)
		FlipToVelocity();

	vMovement = mPos - mOldPos;
	((void (*)(int))FUN_8008b5dc)(Mips_CVecDotProd__FP7CVectorT0(&vMovement, &vMovement));
	TurnAngle = mLeanAngle >> 12;
	mFallLineTurn = 0;

	if (!m720 && !mTapTurn && m1FC && !mNoSpin) {
		if (!TurnAngle) {
			AlignToFallLine(40);
		} else if (mpPad->Square.Pressed || mpPad->Circle.Pressed) {
			m1FC = m1FC - 1;
		} else {
			m1FC = m1FC - 3;

			if (m1FC < 0)
				m1FC = 0;
		}

		TurnAngle += mFallLineTurn;
	} else {
		m1FC = 0;
	}

	Mips_FixMul__Fii(TurnAngle, 3072);
	RotateHeadingAndVelocity((TurnAngle * TimeScale) >> 8, !OnInvisible, 0);

	mTallyAngles += (TurnAngle * TimeScale) >> 8;

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
			if (!Face_Skateable && !Face_WallRide && !Face_Air) {
				int TurnAway;

				mPos = mOldPos;
				TurnAway = 0;

				if (mAnim != 12 && !mInAir && !m368) {
					CVector WallFloorNormal;

					SetVector(&WallFloorNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) if (Mips_CVecDotProd__FP7CVectorT0(&WallFloorNormal, &mFront) >
					                                   2896) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(12, 4120);

						TurnAway = 200;
						SFX_Grunt__FP6CBruce(this);
					}
				}

				ReflectHeadingVelocityInPlane(line_info.Normal, TurnAway);
			} else if (line_info.pItem) {
				if (!Face_Air || Face_Visible) {
					CVector WallFloorNormal;
					int UpDot;

					SetVector(&WallFloorNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) UpDot = Mips_CVecDotProd__FP7CVectorT0(&WallFloorNormal, &mUp);

					if (UpDot > -2048 && UpDot < 2048) {
						int TurnAway = 0;

						if (-line_info.Normal.vy > ABS(line_info.Normal.vx) &&
						    -line_info.Normal.vy > ABS(line_info.Normal.vz)) {
							mSameNormal = 0;
							SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
							          line_info.Normal.vz) mPos.vx =
							    line_info.Position.vx + line_info.Normal.vx * 30;
							mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
							mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;
							OrientToNormal();

							if (mPhysicsState == 2) {
								mSameNormal = 25;
								SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
								          line_info.Normal.vz)
								    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
									          line_info.Normal.vz)
								        ProjectCVectorToPlane(&mFront, line_info.Normal);
								M3dUtils_NormaliseVector(&mFront);
								SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy,
								          line_info.Normal.vz) gte_ldopv1(&mFront);
								gte_ldopv2(&mUp);
								gte_op12();
								gte_stlvnl(&mRight);
								RotMatrixFromFRUVectors();
							}

							StateTransition(PHYSICS_ON_GROUND, 4193);
						} else {
							mPos = mOldPos;
							CVector OldVel = mVel;

							if (mAnim != 12 && OldVel.Length() >= 7 && !mInAir && !m368) {
								CVector WallFloorNormal;

								SetVector(&WallFloorNormal, line_info.Normal.vx,
								          line_info.Normal.vy,
								          line_info.Normal.vz) if (Mips_CVecDotProd__FP7CVectorT0(&WallFloorNormal,
								                                                &mFront) > 2896) {
									mAnimSpeed = 0x10000;
									Debug_RunAnim(12, 4219);
									SFX_Grunt__FP6CBruce(this);

									TurnAway = 200;
								}
							}

							ReflectHeadingVelocityInPlane(line_info.Normal, TurnAway);
							m814.Set(0, 0, 0);
						}
					} else if (line_info.Distance <= Speed >> 12) {
						mPos.vx = line_info.Position.vx + line_info.Normal.vx * 30;
						mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
						mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;

						if (*(int *)&line_info.Normal == *(int *)&mNormal &&
						    line_info.Normal.vz == mNormal.vz) {
							mSameNormal++;
						} else {
							mSameNormal = 0;
							SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
							          line_info.Normal.vz)
						}

						OrientToNormal();
					}
				}
			}
		}
	}

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

	if (mMaxSpeed < Speed)
		m304 = 1;
	else
		m304 = 0;

	CVector Old = mPos;

	if (FUN_800343f0(this, -1)) {
		if (FUN_800343f0(this, 1))
			mPos = Old;
	} else if (FUN_800343f0(this, 1)) {
		if (FUN_800343f0(this, -1))
			mPos = Old;
	}

	FRUVectorsFromRotMatrix();

	line_info.StartCoords.vx = mPos.vx + mUp.vx * 70;
	line_info.StartCoords.vy = mPos.vy + mUp.vy * 70;
	line_info.StartCoords.vz = mPos.vz + mUp.vz * 70;
	line_info.EndCoords.vx = line_info.StartCoords.vx - mUp.vx * 256;
	line_info.EndCoords.vy = line_info.StartCoords.vy - mUp.vy * 256;
	line_info.EndCoords.vz = line_info.StartCoords.vz - mUp.vz * 256;
	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached__FP9SLineInfoi(&line_info, 1);
	GetFaceFlags(&line_info);

	if (line_info.pItem) {
		mTerrain = Face_Terrain;

		if (!Face_Skateable && !Face_WallRide && !Face_Air) {
			StateTransition(PHYSICS_IN_AIR, 4363);
		} else if (line_info.pItem) {
			int d = line_info.Distance;
			int CheckDist;

			if (mPhysicsState == 2) {
				m214 = 20;
				CheckDist = 160;
			} else {
				CheckDist = m214 + 116;
			}

			if (d <= CheckDist) {
				m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;
				CVector Orig = mPos;

				mPos.vx = line_info.Position.vx + mUp.vx * 30;
				mPos.vy = line_info.Position.vy + mUp.vy * 30;
				mPos.vz = line_info.Position.vz + mUp.vz * 30;
				CVector Move = Orig - mPos;

				if (Mips_CVecDotProd__FP7CVectorT0(&Move, &Move) < 4096)
					mPos = Orig;

				if (Face_Air) {
					if (!Face_Visible && m1F8 && XFrames - m1F8 >= 5 && !mInAir && mPhysicsState != 2) {
						SVECTOR N;

						N = line_info.Normal;
						mVel.vx -= Mips_FixMul__Fii(N.vx, Speed / 4);
						mVel.vz -= Mips_FixMul__Fii(N.vz, Speed / 4);
						mVel.vy /= 2;

						if (!mPhysicsState) {
							mNum180Spins = 0;
							mTallyAngles = 0;
						}

						StateTransition(PHYSICS_IN_AIR, 4466);
						FUN_8002f1a0(this);

						m768 = 0;
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 4470);

						m740 = 1;
					} else {
						if (mPhysicsState != 2 && !m740 && !mInAir) {
							mAnimSpeed = 0x10000;
							Debug_RunAnim(14, 4478);

							m740 = 1;
						}

						if (mPhysicsState != 2) {
							FUN_8002f1a0(this);

							mTallyAngles = 0;
							mNum180Spins = 0;
						}

						StateTransition(PHYSICS_ON_INVISIBLE, 4488);
					}
				} else if (Face_Skateable) {
					if (mPhysicsState)
						HandleBailPerpendicular(line_info.Normal);

					if (!mPhysicsState) {
						CVector NewNormal;
						CVector LastNormal;
						int Dot;

						SetVector(&NewNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz)
						    SetVector(&LastNormal, mNormal.vx, mNormal.vy, mNormal.vz)
						        Dot = Mips_CVecDotProd__FP7CVectorT0(&LastNormal, &NewNormal);

						if (Mips_CVecDotProd__FP7CVectorT0(&mFront, &NewNormal) < 0 && mVel.vy <= 0 && Dot > 0 &&
						    Dot < 3000)
							FUN_8003434c(this);
					} else {
						StateTransition(PHYSICS_ON_GROUND, 4551);
					}

					if (!OnInvisible) {
						mpWallItem = line_info.pItem;
						mWallHandle = FUN_8004d7ac(PSXRegion[line_info.pItem->Region].pPSX);
					}
				} else {
					FUN_8003434c(this);
				}

				if (mPhysicsState != 1) {
					if (*(int *)&line_info.Normal == *(int *)&mNormal &&
					    line_info.Normal.vz == mNormal.vz) {
						mSameNormal++;
					} else {
						mSameNormal = 0;
						SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz)
					}

					OrientToNormal();
				}
			} else {
				m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;
				FUN_8003434c(this);
			}
		} else {
			FUN_8003434c(this);
		}
	} else {
		FUN_8003434c(this);
	}

	if (mPhysicsState != 1 && line_info.pItem) {
		if (mPhysicsState == 2)
			InplaneGravity = mGravity;
		else
			InplaneGravity.Set(0, 6500, 0);

		ProjectCVectorToPlane(&InplaneGravity, line_info.Normal);

		mAcc += InplaneGravity;

		if (mPhysicsState != 2)
			mFallLine = InplaneGravity;

		mPolyNormal = line_info.Normal;
		CVectorRotateToPlane(&mVel, &line_info.Normal, &mVel);
	} else {
		mAcc += Gravity;
	}

	if (mPhysicsState != 2 && !mInAir) {
		CVector SideVelocity;
		CVector ForwardsVelocity;
		int dot;

		dot = Mips_CVecDotProd__FP7CVectorT0(&mVel, &mRight);
		SideVelocity.vx = Mips_FixMul__Fii(dot, mRight.vx);
		SideVelocity.vy = Mips_FixMul__Fii(dot, mRight.vy);
		SideVelocity.vz = Mips_FixMul__Fii(dot, mRight.vz);
		mVel -= SideVelocity;
		dot = Mips_CVecDotProd__FP7CVectorT0(&mVel, &mFront);
		ForwardsVelocity.vx = Mips_FixMul__Fii(dot, mFront.vx);
		ForwardsVelocity.vy = Mips_FixMul__Fii(dot, mFront.vy);
		ForwardsVelocity.vz = Mips_FixMul__Fii(dot, mFront.vz);

		if (!m304) {
			if (AutoKickEnabled) {
				mAcc.vx -= Mips_FixMul__Fii(8, ForwardsVelocity.vx);
				mAcc.vy -= Mips_FixMul__Fii(8, ForwardsVelocity.vy);
				mAcc.vz -= Mips_FixMul__Fii(8, ForwardsVelocity.vz);
			}
		} else {
			mAcc.vx -= Mips_FixMul__Fii(82, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul__Fii(82, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul__Fii(82, ForwardsVelocity.vz);
		}

		if (mTerrain == 5) {
			mAcc.vx -= Mips_FixMul__Fii(120, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul__Fii(120, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul__Fii(120, ForwardsVelocity.vz);
		}

		if (mTerrain == 6) {
			mAcc.vx -= Mips_FixMul__Fii(120, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul__Fii(120, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul__Fii(120, ForwardsVelocity.vz);
		}

		if (mTerrain == 3) {
			mAcc.vx -= Mips_FixMul__Fii(180, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul__Fii(180, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul__Fii(180, ForwardsVelocity.vz);
		}
	}
}
