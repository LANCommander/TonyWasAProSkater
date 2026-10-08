/* HandleKick__6CBruce (0x800395b4) */
/* C++ (PHYSICS.CPP). Demo twin; locals from its MAIN.SYM. Pushing off: settles the push anims,
   counts down the kick timer and, when on the ground (or with auto-kick enabled) and not
   blocked, starts a kick (target speed from the skater's stat 1) and, during the kick frames,
   sets the kick impulse along -mFront. GetStat is the shared stat macro (see dofootplant.cpp). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

struct CVector {
	int vx, vy, vz;
};

struct SStats {
	char pad0[0x18];
	int Stats[10]; /* 0x18 */
	char pad40[340 - 0x40];
};

struct SCareer {
	char pad0[0x940];
	int AutoKick[2]; /* 0x940 */
};

struct SColl {
	char pad0[0x10];
	unsigned char x10;
};

extern SStats Stats[];
extern SCareer GCareer;

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int GGame;
extern int GHorsePlayer;

class CBruce {
  public:
	char pad0[0x6c];
	CVector mVel;  /* 0x6c */
	CVector mKick; /* 0x78 */
	char pad84[0x144 - 0x84];
	short mFrame;         /* 0x144 */
	unsigned short mAnim; /* 0x146 */
	char pad148[0x14f - 0x148];
	unsigned char m14F; /* 0x14f */
	int m150;           /* 0x150 */
	char pad154[0x198 - 0x154];
	unsigned char *mpStatBonus; /* 0x198 */
	char pad19c[0x254 - 0x19c];
	int m254; /* 0x254 */
	char pad258[0x2e8 - 0x258];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x318 - 0x2ec];
	int m318; /* 0x318 */
	int m31C; /* 0x31c */
	char pad320[0x338 - 0x320];
	int mKickSpeed; /* 0x338 */
	char pad33c[0x714 - 0x33c];
	int m714; /* 0x714 */
	int m718; /* 0x718 */
	char pad71c[0x734 - 0x71c];
	int m734; /* 0x734 */
	int m738;
	int m73C; /* 0x73c */
	char pad740[0x7d8 - 0x740];
	CVector mFront; /* 0x7d8 */
	char pad7e4[0x7fc - 0x7e4];
	CVector m7FC; /* 0x7fc */
	char pad808[0x80c - 0x808];
	SVECTOR mNormal; /* 0x80c */
	char pad814[0x840 - 0x814];
	int m840; /* 0x840 */
	char pad844[0x86c - 0x844];
	SColl *mpColl; /* 0x86c */
	int m870;      /* 0x870 */
	char pad874[0x918 - 0x874];
	short m918; /* 0x918 */
	void HandleKick();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (Stats[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

void ProjectCVectorToPlane(CVector *, SVECTOR);

extern "C" {
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void Debug_RunAnim1__6CBruceiii(CBruce *, int, int, int);
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
		AutoKickEnabled = GCareer.AutoKick[m870];
	else
		AutoKickEnabled = GCareer.AutoKick[m870 ^ GHorsePlayer];

	if (m734)
		return;

	if (m14F) {
		switch (mAnim) {
			case 1:
				m150 = 0x14000;
				Debug_RunAnim__6CBruceii(this, 3, 6554);
				break;
			case 3:
				m150 = 0x10000;
				Debug_RunAnim__6CBruceii(this, 0, 6559);
				break;
		}
	}

	if (m840 > 0)
		m840--;

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;
	mVel2 = mVel;
	ProjectCVectorToPlane(&mVel2, mNormal);

	x = Mips_CVecDotProd__FP7CVectorT0(&m7FC, &mVel2);

	if (mpColl->x10 || AutoKickEnabled) {
		if (!m318 || !m31C) {
			if (!m840) {
				if ((x >= -1228 && mKickSpeed / 2 < Speed) || m73C) {
					m840 = 20;
				} else if (!m734) {
					switch (mAnim) {
						case 0:
						case 5:
						case 6:
						case 7:
							mKickSpeed = ((GetStat(1) + 17) << 12) * 3 * 15 / 28;
							m840 = 20;
							m150 = 0x14000;
							m714 = 1;
							Debug_RunAnim1__6CBruceiii(this, 1, 0, 6607);
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
					if (mpColl->x10)
						KickVal = 8;

					if (Speed < mKickSpeed && m734 != 2) {
						mKick.vx = -mFront.vx * KickVal;
						mKick.vy = -mFront.vy * KickVal;
						mKick.vz = -mFront.vz * KickVal;
					}
				}
			} else if ((unsigned)(m840 - 11) < 5 && Speed < mKickSpeed && m734 != 2) {
				mKick.vx = -mFront.vx * 4;
				mKick.vy = -mFront.vy * 4;
				mKick.vz = -mFront.vz * 4;
			}
		}

		if (AutoKickEnabled)
			return;
	}

	if (mpColl->x10) {
		if (x >= -1228 && !m73C && !m734) {
			switch (mAnim) {
				case 0:
				case 5:
				case 6:
				case 7:
					mKickSpeed = ((GetStat(1) + 22) << 12) * 3 * 15 / 28;
					m840 = 20;
					m150 = 0x14000;
					Debug_RunAnim1__6CBruceiii(this, 1, 0, 6690);

					m714 = 1;
					break;
			}
		}
	}

	Speed = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

	if (mAnim == 3 || m918 == 2) {
		int KickVal = 4;

		if ((unsigned short)(mFrame - 11) < 5) {
			if (mpColl->x10)
				KickVal = 8;

			if (Speed < mKickSpeed && m734 != 2) {
				mKick.vx = -mFront.vx * KickVal;
				mKick.vy = -mFront.vy * KickVal;
				mKick.vz = -mFront.vz * KickVal;
			}
		}
	}
}
