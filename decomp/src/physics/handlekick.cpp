/* HandleKick__6CBruce (0x800395b4) */
/* C++ (PHYSICS.CPP). Demo twin; locals from its MAIN.SYM. Pushing off: settles the push anims,
   counts down the kick timer and, when on the ground (or with auto-kick enabled) and not
   blocked, starts a kick (target speed from the skater's stat 1) and, during the kick frames,
   sets the kick impulse along -mFront. GetStat is the shared stat macro (see dofootplant.cpp). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern SCareer GCareer;

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int GGame;
extern int GHorsePlayer;

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

void ProjectCVectorToPlane(CVector *, SVECTOR);

extern "C" {
	int SFX_PlayX__FUisi(int, int, int);
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
}

void CBruce::HandleKick() {
	int Speed;
	int AutoKickEnabled;
	CVector mVel2;
	int x;

	if (GGame != 7)
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex];
	else
		AutoKickEnabled = GCareer.Options.AutoKickEnabled[mPadIndex ^ GHorsePlayer];

	if (mPhysicsState)
		return;

	if (mAnimFinished) {
		switch (mAnim) {
			case 1:
				mAnimSpeed = 0x14000;
				Debug_RunAnim(3, 6554);
				break;
			case 3:
				mAnimSpeed = 0x10000;
				Debug_RunAnim(0, 6559);
				break;
		}
	}

	if (m840 > 0)
		m840--;

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;
	mVel2 = mVel;
	ProjectCVectorToPlane(&mVel2, mPolyNormal);

	x = Mips_CVecDotProd__FP7CVectorT0(&mFallLine, &mVel2);

	if (mpPad->Square.Pressed || AutoKickEnabled) {
		if (!mDoingSharpTurn || !mTurning) {
			if (!m840) {
				if ((x >= -1228 && mKickSpeed / 2 < Speed) || mManIsCrouched) {
					m840 = 20;
				} else if (!mPhysicsState) {
					switch (mAnim) {
						case 0:
						case 5:
						case 6:
						case 7:
							mKickSpeed = ((GetStat(1) + 17) << 12) * 3 * 15 / 28;
							m840 = 20;
							mAnimSpeed = 0x14000;
							m714 = 1;
							Debug_RunAnim1(1, 0, 6607);
							break;
					}
				}
			}

			Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

			if (mAnim == 3 || m918 == 2) {
				int KickVal = 4;

				if (mFrame >= 11 && m714) {
					m718 = 34;
					SFX_PlayX__FUisi(34, 0x2000, 0);

					m714 = 0;
				}

				if ((unsigned short)(mFrame - 11) < 5) {
					if (mpPad->Square.Pressed)
						KickVal = 8;

					if (Speed < mKickSpeed && mPhysicsState != 2) {
						mAcc.vx = -mFront.vx * KickVal;
						mAcc.vy = -mFront.vy * KickVal;
						mAcc.vz = -mFront.vz * KickVal;
					}
				}
			} else if ((unsigned)(m840 - 11) < 5 && Speed < mKickSpeed && mPhysicsState != 2) {
				mAcc.vx = -mFront.vx * 4;
				mAcc.vy = -mFront.vy * 4;
				mAcc.vz = -mFront.vz * 4;
			}
		}

		if (AutoKickEnabled)
			return;
	}

	if (mpPad->Square.Pressed) {
		if (x >= -1228 && !mManIsCrouched && !mPhysicsState) {
			switch (mAnim) {
				case 0:
				case 5:
				case 6:
				case 7:
					mKickSpeed = ((GetStat(1) + 22) << 12) * 3 * 15 / 28;
					m840 = 20;
					mAnimSpeed = 0x14000;
					Debug_RunAnim1(1, 0, 6690);

					m714 = 1;
					break;
			}
		}
	}

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

	if (mAnim == 3 || m918 == 2) {
		int KickVal = 4;

		if ((unsigned short)(mFrame - 11) < 5) {
			if (mpPad->Square.Pressed)
				KickVal = 8;

			if (Speed < mKickSpeed && mPhysicsState != 2) {
				mAcc.vx = -mFront.vx * KickVal;
				mAcc.vy = -mFront.vy * KickVal;
				mAcc.vz = -mFront.vz * KickVal;
			}
		}
	}
}
