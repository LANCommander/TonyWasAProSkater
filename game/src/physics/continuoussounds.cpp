/* HandleContinuousSounds__6CBruce (0x8003a6d4) */
/* C++ (PHYSICS.CPP). Demo twin; locals from its MAIN.SYM. Per-frame looping skater sounds:
   the state-5 loop, the state-4 (grind) loop picked by surface, the rolling loop picked by the
   ground terrain (with per-terrain volume/pitch scales), the in-air wind loop (states 1, 2, 8)
   and the speed-driven pitch/volume of the rolling loop. GetStat is the shared stat macro (see
   dofootplant.cpp). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"
#include "game/m3dasm.h"
#include "game/vector.h"

extern SStats Stats[];

extern int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int GLevel;

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])
#define MaxSpeed (((GetStat(1) + 50) << 12) * 3 * 15 / 28)

int SFX_PlayX(unsigned int, short, int);
int SFX_GetVoicePitch(unsigned int);
void SFX_SetVoicePitch(unsigned int, unsigned short);
void SFX_ModifyVol(unsigned int, int, int);
void SFX_Stop(unsigned int);

void CBruce::HandleContinuousSounds() {
	if (mPhysicsState == 5) {
		if (!mLoopSnd)
			mLoopSnd = SFX_PlayX(15, 0x2000, 0);
	} else if (mLoopSnd) {
		SFX_Stop(mLoopSnd);

		mLoopSnd = 0;
	}

	if (mPhysicsState == 4) {
		if (!mGrindSnd) {
			int Snd;

			if (m2EC && (m210 == 0 || m210 == 2))
				Snd = 11;
			else if (!m2EC && (m210 == 0 || m210 == 2))
				Snd = 13;
			else if (GLevel == 8)
				Snd = 170;
			else
				Snd = 12;

			mGrindSnd = SFX_PlayX(Snd, 0x2000, 0);
		}
	} else if (mGrindSnd) {
		SFX_Stop(mGrindSnd);

		mGrindSnd = 0;
	}

	if (mPhysicsState == 0) {
		if (mRollSnd && (mRollTerrain != mTerrain || mInAir)) {
			SFX_Stop(mRollSnd);

			mRollSnd = 0;
		}

		if (!mRollSnd && !mInAir) {
			mRollVolScale = 100;
			mRollPitchScale = 100;
			mRollTerrain = mTerrain;

			switch (mTerrain) {
				case 2:
					mRollSnd = SFX_PlayX(1, 0x2000, 0);
					mRollVolScale = 100;
					mRollPitchScale = 100;
					break;
				case 5:
					mRollSnd = SFX_PlayX(61, 0x2000, 0);
					break;
				case 3:
					mRollSnd = SFX_PlayX(33, 0x2000, 0);
					break;
				case 13:
					mRollSnd = SFX_PlayX(56, 0x2000, 0);
					mRollVolScale = 40;
					mRollPitchScale = 100;
					break;
				case 4:
					if (GLevel == 7) {
						mRollSnd = SFX_PlayX(169, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 110;
					} else {
						mRollSnd = SFX_PlayX(56, 0x2000, 0);
						mRollVolScale = 40;
						mRollPitchScale = 90;
					}

					break;
				case 6:
					mRollSnd = SFX_PlayX(72, 0x2000, 0);
					break;
				case 8:
					mRollSnd = SFX_PlayX(57, 0x2000, 0);
					mRollVolScale = 50;
					mRollPitchScale = 110;
					break;
				case 11:
					mRollSnd = SFX_PlayX(0, 0x2000, 0);
					mRollVolScale = 90;
					mRollPitchScale = 70;
					break;
				case 1:
					if (GLevel == 8) {
						mRollSnd = SFX_PlayX(171, 0x2000, 0);
						mRollVolScale = 150;
						mRollPitchScale = 100;
					} else
						mRollSnd = SFX_PlayX(73, 0x2000, 0);

					break;
				case 12:
					if (GLevel == 8) {
						mRollSnd = SFX_PlayX(143, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 100;
					} else {
						mRollSnd = SFX_PlayX(55, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 100;
					}

					break;
				case 10:
					mRollSnd = SFX_PlayX(71, 0x2000, 0);
					mRollVolScale = 75;
					mRollPitchScale = 150;
					break;
				case 14:
					mRollSnd = SFX_PlayX(105, 0x2000, 0);
					break;
				case 15:
					mRollSnd = SFX_PlayX(119, 0x2000, 0);
					mRollVolScale = 60;
					mRollPitchScale = 150;
					break;
				default:
					mRollSnd = SFX_PlayX(0, 0x2000, 0);
					mRollVolScale = 70;
					break;
			}
		}
	} else if (mRollSnd) {
		SFX_Stop(mRollSnd);

		mRollSnd = 0;
	}

	if (mPhysicsState == 1 || mPhysicsState == 2 || mPhysicsState == 8) {
		if (!mAirSnd) {
			int Spd;
			unsigned short NewPitch;
			unsigned short NewVol;

			Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

			if (mPhysicsState == 8)
				Spd = 300000;

			NewVol = (unsigned short)(Spd * 200 / (MaxSpeed / 16));
			mAirSnd = SFX_PlayX(7, NewVol * 2 / 3, NewVol * 2 / 3);
			SFX_GetVoicePitch(mAirSnd);

			NewPitch = (unsigned short)((Spd * 2048) / MaxSpeed);

			if (NewPitch <= 400)
				NewPitch = 400;

			if (NewPitch >= 1024)
				NewPitch = 1023;

			mAirPitch = NewPitch;
			mAirVol = NewVol;
			SFX_SetVoicePitch(mAirSnd, NewPitch);
		} else if (mAirVol) {
			mAirVol -= (unsigned int)mAirVol >> 5;
			SFX_ModifyVol(mAirSnd, mAirVol, mAirVol);
		}
	} else if (mAirSnd) {
		SFX_Stop(mAirSnd);

		mAirSnd = 0;
	}

	if (mRollSnd) {
		int Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

		SFX_GetVoicePitch(mRollSnd);
		int NewPitch = (Spd * 2048) / MaxSpeed;
		int NewVol = Spd * 2024 / (MaxSpeed / 16);

		NewVol = NewVol * mRollVolScale / 100;
		SFX_ModifyVol(mRollSnd, NewVol, NewVol);

		if (NewPitch <= 700)
			NewPitch = 700;

		if (NewPitch >= 1024)
			NewPitch = 1023;

		NewPitch = NewPitch * mRollPitchScale / 100;
		SFX_SetVoicePitch(mRollSnd, NewPitch);

		mAirPitch = NewPitch;
		mAirVol = NewVol;
	}
}
