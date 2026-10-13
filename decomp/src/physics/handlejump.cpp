/* HandleJump__6CBruce (0x80038268) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. Ollie handling: settles the crouch
   anims, plays the landing sound/vibration and scores the landing, counts the crouch (ollie
   charge) timer while on the ground, and on release launches the skater: off a rail, along the
   rail; off a wall-ride, away from the wall; the upward kick scales with the charge and the
   skater's stats. A quick double-tap within the window does a boneless / no-comply.
   GetStat is the shared stat macro (see dofootplant.cpp). Levers: SFX_PlayX (SFX_PlayX__FUisi) is
   declared void here (v0 free for the |mUp.vy| test); the two JumpBoost arms share their tail by
   cross-jumping, so the boost is a local added once; the early-out call is wrapped in
   do { } while (0) (keeps `sw mJumpPending` ahead of the argument setup). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}

extern SStats Stats[];

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int XFrames;
extern int GState;
extern char s_FASTPLANT[];
extern char s_NOLLIE[];

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])

int Redbook_Rnd(int);

extern "C" {
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
	int Mips_FixMul__Fii(int, int);
	void SFX_PlayX__FUisi(int, int, int);
	int FUN_8006ebdc(CPanelData *);
	void FUN_8002f1a0(CBruce *);
	void FUN_8002f458(CBruce *, int);
}

void CBruce::HandleJump() {
	int Speed;

	if (mInAir)
		return;

	if (mManIsCrouched) {
		if (!mAnim) {
			mAnimSpeed = 0x10000;
			Debug_RunAnim3(8, 0, 26, 19, 6105);
		}
	} else if (mAnim == 8) {
		mAnimSpeed = 0x10000;

		if (mTargetFrame2 == -1) {
			if (!mFrame)
				Debug_RunAnim(0, 6114);
		} else
			Debug_RunAnim2(8, 19, 0, 6117);
	}

	if (mAnimFinished) {
		switch (mAnim) {
			case 8:
				if (!mFrame) {
					mAnimSpeed = 0x10000;
					Debug_RunAnim(0, 6130);
				}

				break;
			case 5:
			case 25:
				if (mManIsCrouched) {
					mAnimSpeed = 0x10000;
					Debug_RunAnim3(8, 19, 26, 19, 6145);
				} else {
					mAnimSpeed = 0x10000;
					Debug_RunAnim(0, 6150);
				}

				break;
			case 4:
			case 13:
				if (!mManIsCrouched) {
					mAnimSpeed = 0x10000;
					Debug_RunAnim(14, 6160);
				}

				break;
			case 14:
				mAnimSpeed = 0x10000;
				Debug_RunAnim(14, 6166);
				break;
			case 34:
				mAnimSpeed = 0x10000;

				if (mManIsCrouched)
					Debug_RunAnim3(8, 19, 26, 19, 6175);
				else
					Debug_RunAnim2(8, 19, 0, 6178);

				break;
			case 0:
				mAnimSpeed = 0x10000;

				if (mManIsCrouched)
					Debug_RunAnim3(8, 19, 26, 19, 6191);
				else
					Debug_RunAnim(0, 6194);

				break;
		}
	}

	if (m740 && mOldPhysicsState && !mPhysicsState) {
		m740 = 0;
		m764 = 0;

		if (!mInAir) {
			switch (mTerrain) {
				case 2:
					SFX_PlayX__FUisi(Redbook_Rnd(2) + 38, 0x2000, 0);
					break;
				case 10:
				case 12:
					SFX_PlayX__FUisi(35, 0x2000, 0);
					break;
				case 6:
					break;
				case 1:
				case 15:
				default:
					SFX_PlayX__FUisi(5, 0x2000, 0);
					break;
			}

			Vibrate(2, 1, 255);

			if (mManIsCrouched) {
				mAnimSpeed = 0x10000;
				Debug_RunAnim1(25, 3, 6253);
			} else {
				mAnimSpeed = 0x10000;
				Debug_RunAnim1(5, 0, 6258);
			}

			HandleBailOnLanding(6261);

			if (!mInAir) {
				/* PARKED FOR GOOD, inline asm (end-game policy). Retail evaluates TotalScore()/10
				   here and keeps only its `mult`: the lone dead mult in the game. The original was
				   presumably `int Score = TotalScore() / 10;` (unused). Psy-Q 4.5 can't leave just the
				   mult: the only pass that would (post-reload delete_computation) is disabled under
				   -fschedule-insns2 (egcs jump.c:3658). Earlier passes delete the whole chain or
				   leave the x>>31 sra. See the README "Still unexplained" note. */
				__asm__ volatile("lui $3,0x6666; ori $3,$3,0x6667; mult %0,$3"
				                 :
				                 : "r"(FUN_8006ebdc(&mPanel))
				                 : "$3");
				int Bonus = FUN_8006ebdc(&mPanel) / 100;

				if (Bonus > 20)
					Bonus = 20;

				m340 = 30;
				m344 = 30;
				m33C += Bonus;
				FUN_8002f458(this, Bonus);
				Trick_Land();
			}
		}

		mDoingATrick = 0;
	}

	if (mpPad->Ollie.Pressed && !mInAir) {
		mJumpStrength++;

		if (mJumpStrength > 15 - (GetStat(0) + GetStat(2)) / 2)
			mJumpStrength = 15 - (GetStat(0) + GetStat(2)) / 2;

		if (mPhysicsState == 0 && !m310) {
			mJumpPending = 1;

			if (!mManIsCrouched) {
				mAnimSpeed = 0x10000;

				if (ABS(mLeanAngle) <= 0x9fff) {
					mLeanAngle = 0;
					Debug_RunAnim3(8, 0, 26, 19, 6331);
				}
			}
		} else if (mPhysicsState == 4 && !m310) {
			mJumpPending = 1;
		} else if (mPhysicsState == 5 && !m310) {
			mJumpPending = 1;
		}

		if (!m310)
			mManIsCrouched = 1;
	} else {
		m310 = 0;
		mManIsCrouched = 0;

		if (m300 > 0 && XFrames - m300 > 20)
			mJumpPending = 0;

		if (mJumpPending) {
			int LastPhysics;
			int JumpBoost;

			if (GState == 1 && mPhysicsState != 4 && mPhysicsState != 5) {
				mJumpPending = 0;

				do {
					Debug_RunAnim(0, 6362);
				} while (0);

				mJumpStrength = 0;
				return;
			}

			mJumpPending = 0;
			m740 = 1;
			m74C = mJumpStrength;
			m2F8 = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

			if (m6DC) {
				int a;

				m70C = 5;
				m6DC = 0;
				mStickToRail = 0;
				mDoingATrick = 0;
				mFront = mRailDir;
				AlignHeadingOnRail();

				Speed = Mips_CVecDotProd__FP7CVectorT0(&mRailDir, &mVel);
				mVel.vx = Mips_FixMul__Fii(mRailDir.vx, Speed);
				mVel.vy = Mips_FixMul__Fii(mRailDir.vy, Speed);
				mVel.vz = Mips_FixMul__Fii(mRailDir.vz, Speed);
				a = 0;

				if (mpPad->Left.Pressed)
					a = -200;

				if (mpPad->Right.Pressed)
					a = 200;

				RotateHeadingAndVelocity(a, 1, 0);
			}

			mAnimSpeed = 0x10000;
			m764 = m768;
			m754 = mPhysicsState == 5;
			LastPhysics = mPhysicsState;

			if (LastPhysics == 5) {
				CVector SurfaceNormal;

				mJumpStrength = 15 - (GetStat(0) + GetStat(2)) / 2;
				SetVector(&SurfaceNormal, mPolyNormal.vx, mPolyNormal.vy, mPolyNormal.vz) mAcc +=
				    SurfaceNormal * 2;
				mVel.vy -= 0xf000;
			}

			{
				int Sfx = 6;

				if (mPhysicsState == 4 && !m2EC) {
					Sfx = 16;

					if (m210 != 1)
						Sfx = 6;
				}

				SFX_PlayX__FUisi(Sfx, 0x2000, 0);
			}

			if (ABS(mUp.vy) < 2500)
				JumpBoost =
				    -((mJumpStrength *
					       (((32 - (10 - GetStat(2)) / 2) + ((10 - GetStat(2)) / 3 - 18)) << 10) /
					       (15 - (GetStat(0) + GetStat(2)) / 2) +
					   ((32 - (10 - GetStat(2)) / 2) << 10)) *
					  3);
			else
				JumpBoost =
				    -((mJumpStrength *
					       (((32 - (10 - GetStat(0)) / 2) + ((10 - GetStat(0)) / 3 - 18)) << 10) /
					       (15 - (GetStat(0) + GetStat(2)) / 2) +
					   ((32 - (10 - GetStat(0)) / 2) << 10)) *
					  3);

			mVel.vy += JumpBoost;
			mJumps++;

			if (mJumpStrength < 15 - (GetStat(0) + GetStat(2)) / 2)
				mWeakJumps++;

			mTallyAngles = 0;
			mNum180Spins = 0;

			if (mPhysicsState != 2) {
				if (m764 && mPhysicsState == 0) {
					StateTransition(PHYSICS_IN_AIR_STICK_TO, 6480);

					m1E8 = XFrames;
				} else
					StateTransition(PHYSICS_IN_AIR, 6485);
			}

			if (!m368 || (m368 & 0xf000) == 0x1000) {
				mAnimSpeed = 0x10000;

				if (LastPhysics != 5 && m278 == 1 && XFrames - m274 < 20) {
					mTallyAngles = 0;
					mNum180Spins = 0;

					if (m268 == 1 && XFrames - m26C < 35) {
						FUN_8002f1a0(this);
						Trick_Trick(65, s_FASTPLANT, 250, 0);

						mVel.vy -= 0x5000;
						Debug_RunAnim1(45, 16, 6508);
					} else {
						FUN_8002f1a0(this);
						Trick_Trick(75, s_NOLLIE, 200, 0);
						Debug_RunAnim(13, 6515);
					}

					m364 = 1;
				} else {
					FUN_8002f1a0(this);
					Debug_RunAnim(4, 6523);
				}
			}

			m2FC = XFrames;
		}

		mJumpStrength = 0;
	}
}
