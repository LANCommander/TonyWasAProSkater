/* DoPhysics__6CBruce (0x8003b424) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). THPS1's per-frame skater update. The THPS2 demo split most of this into
   helpers (HandleTimer, HandleFlips, HandleBraking, HandlePhysicsStates, HandleBailStates,
   CheckForTriggers, LimitFinalSpeed); THPS1 still has them written out in DoPhysics. The
   `Debug_RunAnim` line arguments (7869..8495) are this file's __LINE__s.
   Levers: -fstructure-noalias; the 44-byte trick records are separate globals (one lui each);
   the dead stack Gravity is stored field by field and sits between the two Set()s (its
   constant 13000 is shared with mGravity.Set); the unused locals give the 248-byte gap below it;
   `check` is a function-scope C++ local declared after UberFrig, so it takes the freed arg
   slot and stays live, pushing the final mAcc temps up 16; `old` is initialised, not assigned
   (no copy temp); BrakeVal = 4096 - Angle; BrakeVal = BrakeVal * 6144 / 208; puts the
   difference in BrakeVal's s0; the anim and state range tests are switches (slti trees). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
#include "game/colij.h"
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern SCareer GCareer;
extern SLevel Levels[];
extern STrick DAT_800a7250, DAT_800a727c, DAT_800a72a8, DAT_800a72d4, DAT_800a7300, DAT_800a732c;
extern STrick DAT_800a7460, DAT_800a74e4, DAT_800a7510;
extern short HeadBloodAtFrame[];
extern CVector WorldUp;
extern CVector DAT_800d25e4;

int Cheat_StatsOverride;
int DAT_800d1950;

extern int Cheat_Fire;
extern int GGame;
extern int GAutoTest;
extern int EndCode;
extern int XFrames;
extern int TimeScale;
extern int GState;
extern int GHorsePlayer;
extern int GLevel;
extern int BaddyCollisionCheck;
extern int BulletCollisionCheck;

extern CBruce *GSkater;
extern CBruce *pCurrentPlayer;

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])
#define MAXSPEED(k) (((GetStat(1) + (k)) << 12) * 3 * 15 / 28)

void UberFrig(CBruce *, CVector, char *);
void Utils_CalcUnit(CVector &, const CVector &);
void M3dColij_InitLineInfo(SLineInfo *);
void Trig_ResetCPCollisionFlags(int);
void Trig_ResetCPExecutedFlags(int);
int SFX_PlayX(unsigned int, short, int);

extern "C" {
	void FUN_80042700(CBruce *, int, int, int, int);
	void Game_Over__Fv(void);
	void FUN_8003b2f0(CBruce *);
	int M3dZone_LineToItemNotSCached__FP9SLineInfoi(SLineInfo *, int);
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
}

void CBruce::DoPhysics() {
	SLineInfo line_info;
	CVector unused1, unused2, unused3, unused4, unused5;
	int AutoKickEnabled;
	int Spd;

	if (GGame != 7)
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex];
	else
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex ^ GHorsePlayer];

	WorldUp.Set(0, -4096, 0);
	CVector Gravity;

	Gravity.vx = 0;
	Gravity.vy = 13000;
	Gravity.vz = 0;
	mGravity.Set(0, 13000, 0);

	switch (Stats[mSkaterIndex].Stats[4]) {
		case 0:
			DAT_800a74e4 = DAT_800a727c;
			DAT_800a7510 = DAT_800a72a8;
			DAT_800a7460 = DAT_800a7250;
			break;
		case 1:
			DAT_800a74e4 = DAT_800a7300;
			DAT_800a7510 = DAT_800a732c;
			DAT_800a7460 = DAT_800a72d4;
			break;
		case 2:
			DAT_800a74e4 = DAT_800a7250;
			DAT_800a7510 = DAT_800a72a8;
			DAT_800a7460 = DAT_800a72d4;
			break;
		case 3:
			DAT_800a74e4 = DAT_800a727c;
			DAT_800a7510 = DAT_800a732c;
			DAT_800a7460 = DAT_800a7250;
			break;
		case 4:
			DAT_800a74e4 = DAT_800a727c;
			DAT_800a7510 = DAT_800a732c;
			DAT_800a7460 = DAT_800a7250;
			break;
	}

	if (m214) {
		m214 -= 5;

		if (m214 < 0)
			m214 = 0;
	}

	if (mBalanceTime) {
		mBalanceTime -= 2;

		if (mBalanceTime <= 0)
			mBalanceTime = 0;
	}

	if (mBalanceFlash) {
		mBalanceFlash -= 2;

		if (mBalanceFlash <= 0) {
			mBalanceFlash = 0;
			FUN_80042700(this, 0, 0, 0, 5);
		}
	}

	if ((mpPad->Up.Pressed || mAY < -40) && !mpPad->Left.Pressed && !mpPad->Right.Pressed && !mAX) {
		if (!m1F8)
			m1F8 = XFrames;
	} else
		m1F8 = 0;

	m1A4 = mVel;
	BaddyCollisionCheck = 1;
	BulletCollisionCheck = 0;
	Trick_PlayerInit(0);

	/* HandleTimer */
	if (GGame != 2) {
		int ticks;

		if (GAutoTest)
			ticks = 100 - XFrames;
		else
			ticks = mTime - XFrames;

		if (ticks < 0 && !GState) {
			if (GAutoTest)
				EndCode = 8;
			else {
				if (GGame == 1 && Levels[GLevel].isCompetition)
					SFX_PlayX(160, 0x2000, 0);

				Game_Over__Fv();
			}
		}
	}

	/* HandleFlips */
	if (m720) {
		int old = m724;

		m724 = old + ((m728 * TimeScale) >> 8);

		if ((m724 ^ old) & 0x1000) {
			m720--;

			if (!m720) {
				m724 = 0;
				m728 = 0;
			}
		}
	}

	FRUVectorsFromRotMatrix();
	HandleSpin();

	if (mPhysicsState != 1)
		mLastCar = 0;

	mAcc.vx = mAcc.vy = mAcc.vz = 0;
	HandleKick();
	HandleJump();

	if (mPhysicsState != 5 && mAnimFinished) {
		switch (mAnim) {
			case 35:
			case 36:
				mAnimSpeed = 0x10000;

				if (!mPhysicsState)
					Debug_RunAnim(0, 7869);
				else
					Debug_RunAnim(14, 7871);

				break;
		}
	}

	HandleTricksNew();

	/* HandleBraking */
	{
		int DownOnly = 0;

		if ((mpPad->Down.Pressed && !mpPad->Left.Pressed && !mpPad->Right.Pressed) ||
		    (mAY > 0 && !mAX))
			DownOnly = 1;

		if (mPhysicsState == 7 && !DownOnly)
			StateTransition(PHYSICS_ON_GROUND, 7888);

		if (!mPhysicsState && DownOnly && !mInAir) {
			int Angle = -(mPolyNormal.vy << 12) >> 12;
			int BrakeVal;

			if (Angle < 768)
				Angle = 768;

			BrakeVal = 4096 - Angle;
			BrakeVal = BrakeVal * 6144 / 208;

			if (BrakeVal < 0xa000)
				BrakeVal = 0xa000;

			if (BrakeVal < FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6) {
				if (mAnim != 49) {
					mAnimSpeed = 0x20000;
					SFX_PlayX(2, 0x2000, 0);
					Debug_RunAnim(49, 7923);
				}

				if (mAY > 0) {
					int b = mAY >> 4;

					mVel.vx -= ((mVel.vx * b / 128) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy * b / 128) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz * b / 128) * TimeScale) >> 8;
				} else {
					mVel.vx -= ((mVel.vx >> 4) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 4) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 4) * TimeScale) >> 8;
				}
			} else if (BrakeVal <= 0xa000) {
				StateTransition(PHYSICS_STOPPED, 7943);
				mVel.Set(0, 0, 0);

				if (mAnim == 49)
					Debug_RunAnim(0, 7984);
			}

			m840 = 2;
		} else if (mAnim == 49) {
			mAnimSpeed = 0x10000;
			Debug_RunAnim(0, 7996);
		}
	}

	if (mAnimFinished) {
		switch (mAnim) {
			case 11:
			case 12:
				if (mManIsCrouched) {
					mAnimSpeed = 0x10000;
					Debug_RunAnim3(8, 19, 26, 19, 8012);
				} else {
					mAnimSpeed = 0x10000;
					Debug_RunAnim(0, 8017);
				}

				break;
		}
	}

	mOldPhysicsState = mPhysicsState;
	mOldRotMatrix = mRotMatrix;
	m1B0 = mOldPos;
	mOldPos = mPos;
	m814.Set(0, 0, 0);
	HandleContinuousSounds();

	if (mManIsCrouched)
		mKickSpeed = MAXSPEED(22);

	if (m33C) {
		int NewMag = (FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6) >> 12;

		mAcc.vx += mVel.vx / NewMag;
		mAcc.vy += mVel.vy / NewMag;
		mAcc.vz += mVel.vz / NewMag;
		m33C--;
	}

	if (mMaxSpeed < FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6)
		m304 = 1;
	else
		m304 = 0;

	if (m340 > 0)
		m340--;
	else if (m344 > 0)
		m344--;
	else {
		m344 = 30;

		if (MAXSPEED(39) < mMaxSpeed)
			mMaxSpeed -= 4096;

		if (MAXSPEED(17) < mKickSpeed)
			mKickSpeed -= 4096;
	}

	UberFrig(this, mPos, "restart at start of main physics loop");

	mSafePos = mPos;

	if (!mInAir)
		BoardOn();

	if (mPhysicsState != 8)
		HandleStickToRail();

	if (mInAir && mAnim == 46 && mFrame == HeadBloodAtFrame[mSkaterIndex])
		BloodSplat(0, 50);

	/* HandlePhysicsStates */
	mRailTime++;

	switch (mPhysicsState) {
		case 0:
			mBalance = 0;
			mBalanceVel = 0;
			m8EC = 0;
			mRailTime = 0;
			mGrinds = 0;
			mGaps = 0;
			mpGrindGapTrick = 0;
			m1DC = mPos;
			DoOnGroundPhysics();
			break;
		case 7:
			DoOnGroundPhysics();

			mPos = mOldPos;
			break;
		case 4: {
			int i;

			if (mpGrindGapTrick) {
				Trick_Trick(77, mpGrindGapTrick->Name, -mpGrindGapTrick->Score, 0);
				SFX_PlayX(161, 0x2000, 0);

				mpGrindGapTrick = 0;
			}

			for (i = 0; i < 10; i++)
				if (mGapType[i] < 10000)
					mGapChecksum[i] = 12345;

			DoOnRailPhysics();
		} break;
		case 8:
			DoHandPlantPhysics();
			break;
		case 6:
			DoFootPlantPhysics();
		case 1:
			DoInAirPhysics();
			break;
		case 5:
			DoWallRidePhysics();
			break;
		case 3:
			DoInAirPhysics();

			if (mPhysicsState == 3 && XFrames - m1E8 >= 26)
				StateTransition(PHYSICS_IN_AIR, 8263);

			break;
		case 2:
			DoOnGroundPhysics();
			break;
	}

	/* HandleBailStates */
	if (mInAir) {
		mAcc.vx = 0;
		mAcc.vz = 0;
		mJumpPending = 0;
		m740 = 0;

		if ((!mBailNumber || mPhysicsState == 2) && mVel.vy < 0)
			mVel.vy = 0;

		switch (m24C) {
			case 1:
				if (mFrame >= 15 && !m71C && !mPhysicsState) {
					Vibrate(4, 1, 255);
					SFX_PlayX(38, 0x2000, 0);
					BloodSplat(0, 50);

					m71C = 1;
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (mFrame >= 16 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 5) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 5) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 5) * TimeScale) >> 8;
				}

				if (mAnimFinished && !mPhysicsState) {
					if (mAnim != 46)
						m24C = 2;
					else
						m24C = 4;
				}

				break;
			case 6:
				if (mFrame >= 15 && !m71C && !mPhysicsState) {
					Vibrate(4, 1, 255);
					SFX_PlayX(38, 0x2000, 0);
					BloodSplat(0, 50);

					m71C = 1;
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (mFrame >= 16 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 5) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 5) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 5) * TimeScale) >> 8;
				}

				if (mAnimFinished && !mPhysicsState) {
					if (mAnim != 46) {
						Debug_RunAnim(55, 8366);

						m24C = 7;
					} else
						m24C = 4;
				}

				break;
			case 7:
				if (!mPhysicsState) {
					mVel.vx = 0;
					mVel.vy = 0;
					mVel.vz = 0;
				}

				if (mAnimFinished && !mPhysicsState) {
					Debug_RunAnim1(54, 99, 8381);

					m24C = 4;
				}

				break;
			case 2:
				if (!mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 5) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 5) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 5) * TimeScale) >> 8;
				}

				mAnimSpeed = 0x10000;
				Debug_RunAnim1(54, 30, 8399);

				m24C = 3;
				break;
			case 5:
				if (mFrame >= 26 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				} else if (mFrame >= 34 && !mPhysicsState) {
					mVel.vx = 0;
					mVel.vy = 0;
					mVel.vz = 0;
				}

				if (mAnimFinished && !mPhysicsState)
					m24C = 4;

				break;
			case 3:
				if (mFrame >= 51 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (mAnimFinished && !mPhysicsState)
					m24C = 4;

				break;
			case 8:
				if (mFrame < 18) {
					mAcc.vy = 0;
					mVel.vx -= ((mVel.vx >> 4) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 4) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 4) * TimeScale) >> 8;
				}

				if (mAnimFinished && !mPhysicsState) {
					Debug_RunAnim1(54, 76, 8451);

					m24C = 4;
				}

				break;
			case 9:
				if (mFrame >= 29 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (mAnimFinished && !mPhysicsState) {
					Debug_RunAnim1(46, 36, 8467);

					m24C = 4;
				}

				break;
			case 4:
				if (!mPhysicsState) {
					mVel.vx = 0;
					mVel.vy = 0;
					mVel.vz = 0;
				}

				if (mFrame >= 112)
					BoardOn();

				if (mAnimFinished) {
					m840 = 4;
					mAnimSpeed = 0x10000;

					if (mManIsCrouched && mpPad->Flip.Pressed)
						Debug_RunAnim3(8, 0, 26, 19, 8490);
					else {
						mManIsCrouched = 0;
						Debug_RunAnim(0, 8495);
					}

					mInAir = 0;
					mVel.vx = mVel.vy = mVel.vz = 0;
					BoardOn();
				}

				break;
		}
	}

	FUN_8003b2f0(this);

	/* CheckForTriggers */
	if (!DAT_800d1950)
		DAT_800d1950 = 1;

	CVector check = mPos - mOldPos;

	if (check.vx | check.vy | check.vz) {
		SLineInfo LInfo;
		CVector unit;
		int n;

		DAT_800d25e4 = check;
		Utils_CalcUnit(unit, DAT_800d25e4);
		CVector old = mOldPos - unit * 16;

		LInfo.StartCoords.vx = old.vx;
		LInfo.StartCoords.vy = old.vy;
		LInfo.StartCoords.vz = old.vz;
		LInfo.EndCoords.vx = mPos.vx;
		LInfo.EndCoords.vy = mPos.vy;
		LInfo.EndCoords.vz = mPos.vz;
		M3dColij_InitLineInfo(&LInfo);

		LInfo.RecordTriggerZoneHits = 1;
		n = this == GSkater ? 1 : 2;
		Trig_ResetCPCollisionFlags(n);

		pCurrentPlayer = this;
		M3dZone_LineToItemNotSCached__FP9SLineInfoi(&LInfo, 1);
		Trig_ResetCPExecutedFlags(n);

		pCurrentPlayer = 0;
	}

	switch (mPhysicsState) {
		case 1:
		case 2:
		case 3:
			if (!m300)
				m300 = XFrames;

			break;
		default:
			m300 = 0;
			break;
	}

	mVel += (mAcc * TimeScale) >> 8;

	/* LimitFinalSpeed */
	Spd = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

	if (Spd > MAXSPEED(50)) {
		mVel.vx = mVel.vx * (MAXSPEED(50) >> 12) / (Spd >> 12);
		mVel.vy = mVel.vy * (MAXSPEED(50) >> 12) / (Spd >> 12);
		mVel.vz = mVel.vz * (MAXSPEED(50) >> 12) / (Spd >> 12);
		Spd = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;
	}

	if (!AutoKickEnabled) {
		if (ABS(Spd) < 0x10000) {
			mVel.vx -= mVel.vx / 32;
			mVel.vy -= mVel.vy / 32;
			mVel.vz -= mVel.vz / 32;
		}

		if (ABS(Spd) < 0x2000) {
			mVel.vx -= mVel.vx / 4;

			if (ABS(mVel.vx) < 16)
				mVel.vx = 0;

			mVel.vy -= mVel.vy / 4;

			if (ABS(mVel.vy) < 16)
				mVel.vy = 0;

			mVel.vz -= mVel.vz / 4;

			if (ABS(mVel.vz) < 16)
				mVel.vz = 0;
		}
	}
}
