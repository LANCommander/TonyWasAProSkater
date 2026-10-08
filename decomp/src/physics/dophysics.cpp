/* DoPhysics__6CBruce (0x8003b424) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). THPS1's per-frame skater update. The THPS2 demo split most of this into
   helpers (HandleTimer, HandleFlips, HandleBraking, HandlePhysicsStates, HandleBailStates,
   CheckForTriggers, LimitFinalSpeed); THPS1 still has them written out in DoPhysics. The
   `Debug_RunAnim` line arguments (7869..8495) are this file's __LINE__s.
   Levers: -fstructure-noalias; the 44-byte trick records are separate globals (one lui each);
   the dead stack Gravity is stored field by field and sits between the two Set()s (its
   constant 13000 is shared with mGravity.Set); the unused locals give the 248-byte gap below it;
   `check` is a function-scope C++ local declared after UberFrig, so it takes the freed arg
   slot and stays live, pushing the final mKick temps up 16; `old` is initialised, not assigned
   (no copy temp); BrakeVal = 4096 - Angle; BrakeVal = BrakeVal * 6144 / 208; puts the
   difference in BrakeVal's s0; the anim and state range tests are switches (slti trees). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

typedef unsigned short ushort;
typedef unsigned char uchar;

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

struct CVector {
	int vx, vy, vz;
	void Set(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
	CVector &operator+=(const CVector &);
};

CVector operator*(const CVector &, const int &);
CVector operator>>(const CVector &, const int &);
CVector operator-(const CVector &, const CVector &);

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle; /* 0x00 */
	SButton Square;   /* 0x10 */
	SButton Circle;   /* 0x20 */
	SButton X;        /* 0x30 */
	SButton LeftOne;  /* 0x40 */
	SButton LeftTwo;  /* 0x50 */
	SButton RightOne; /* 0x60 */
	SButton RightTwo; /* 0x70 */
	SButton Left;     /* 0x80 */
	SButton Right;    /* 0x90 */
	SButton Up;       /* 0xa0 */
	SButton Down;     /* 0xb0 */
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

struct SLevel {
	char pad0[0xc];
	int f0C; /* 0x0c */
	char pad10[120 - 0x10];
};

struct STrickInfo {
	int w[11];
};

struct STrickScore {
	short pad0;
	short Score; /* 0x2 */
	char *pName; /* 0x4 */
};

struct SLineInfo {
	CVector StartCoords;
	CVector EndCoords;
	char pad18[0x88 - 0x18];
	uchar Flag88; /* 0x88 */
	char pad89[168 - 0x89];
};

extern SStats Stats[];
extern SCareer GCareer;
extern SLevel Levels[];
extern STrickInfo DAT_800a7250, DAT_800a727c, DAT_800a72a8, DAT_800a72d4, DAT_800a7300,
    DAT_800a732c;
extern STrickInfo DAT_800a7460, DAT_800a74e4, DAT_800a7510;
extern short HeadBloodAtFrame[];
extern CVector WorldUp;
extern CVector DAT_800d25e4;
extern char s_restart_at_start_of_main_physics[];

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

class CBruce;

extern CBruce *GSkater;
extern CBruce *pCurrentPlayer;

class CBruce {
  public:
	char pad0[0x4];
	CVector mPos; /* 0x4 */
	char pad10[0x6c - 0x10];
	CVector mVel;  /* 0x6c */
	CVector mKick; /* 0x78 */
	char pad84[0xf0 - 0x84];
	CVector mOldPos; /* 0xf0 */
	char padfc[0x144 - 0xfc];
	short mFrame; /* 0x144 */
	ushort mAnim; /* 0x146 */
	char pad148[0x14f - 0x148];
	uchar m14F; /* 0x14f */
	int m150;   /* 0x150 */
	char pad154[0x198 - 0x154];
	uchar *mpStatBonus; /* 0x198 */
	int m19C;           /* 0x19c */
	int m1A0;           /* 0x1a0 */
	CVector m1A4;       /* 0x1a4 */
	CVector m1B0;       /* 0x1b0 */
	CVector m1BC;       /* 0x1bc */
	CVector mGravity;   /* 0x1c8 */
	char pad1d4[0x1dc - 0x1d4];
	CVector m1DC; /* 0x1dc */
	int m1E8;     /* 0x1e8 */
	char pad1ec[0x1f8 - 0x1ec];
	int m1F8; /* 0x1f8 */
	char pad1fc[0x20c - 0x1fc];
	int m20C; /* 0x20c */
	char pad210[0x214 - 0x210];
	int m214; /* 0x214 */
	char pad218[0x24c - 0x218];
	int m24C; /* 0x24c */
	int m250; /* 0x250 */
	int m254; /* 0x254 */
	char pad258[0x28c - 0x258];
	int m28C[10];      /* 0x28c */
	int m2B4[10];      /* 0x2b4 */
	int m2DC;          /* 0x2dc */
	STrickScore *m2E0; /* 0x2e0 */
	char pad2e4[0x2e8 - 0x2e4];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x300 - 0x2ec];
	int m300; /* 0x300 */
	int m304; /* 0x304 */
	char pad308[0x330 - 0x308];
	int m330;       /* 0x330 */
	int m334;       /* 0x334 */
	int mKickSpeed; /* 0x338 */
	int m33C;       /* 0x33c */
	int m340;       /* 0x340 */
	int m344;       /* 0x344 */
	char pad348[0x358 - 0x348];
	int m358; /* 0x358 */
	char pad35c[0x71c - 0x35c];
	int m71C; /* 0x71c */
	int m720; /* 0x720 */
	int m724; /* 0x724 */
	int m728; /* 0x728 */
	char pad72c[0x734 - 0x72c];
	int mPhysicsState;  /* 0x734 */
	int m738;           /* 0x738 */
	int mManIsCrouched; /* 0x73c */
	int m740;           /* 0x740 */
	int m744;           /* 0x744 */
	char pad748[0x76c - 0x748];
	MATRIX mOldRotMatrix; /* 0x76c */
	MATRIX mRotMatrix;    /* 0x78c */
	char pad7ac[0x80c - 0x7ac];
	SVECTOR mNormal; /* 0x80c */
	CVector m814;    /* 0x814 */
	char pad820[0x840 - 0x820];
	int m840; /* 0x840 */
	char pad844[0x86c - 0x844];
	SControl *mPad; /* 0x86c */
	int m870;       /* 0x870 */
	char pad874[0x894 - 0x874];
	signed char mAX; /* 0x894 */
	signed char mAY; /* 0x895 */
	char pad896[0x8e4 - 0x896];
	int mBalance;    /* 0x8e4 */
	int mBalanceVel; /* 0x8e8 */
	int m8EC;        /* 0x8ec */
	int mRailTime;   /* 0x8f0 */
	void Trick_PlayerInit(ushort);
	void Trick_Trick(int, char *, int, int);
	void StateTransition(int, int);
	void Vibrate(int, int, int);
	void BloodSplat(int, int);
	void FRUVectorsFromRotMatrix();
	void HandleSpin();
	void HandleKick();
	void HandleJump();
	void HandleTricksNew();
	void HandleContinuousSounds();
	void HandleStickToRail();
	void DoOnRailPhysics();
	void DoHandPlantPhysics();
	void DoFootPlantPhysics();
	void DoInAirPhysics();
	void DoWallRidePhysics();
	void DoPhysics();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (Stats[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
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
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void Debug_RunAnim1__6CBruceiii(CBruce *, int, int, int);
	void Debug_RunAnim3__6CBruceiiiii(CBruce *, int, int, int, int, int);
	void BoardOn__6CBruce(CBruce *);
	void DoOnGroundPhysics__6CBruce(CBruce *);
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
		AutoKickEnabled = GCareer.AutoKick[m870];
	else
		AutoKickEnabled = GCareer.AutoKick[m870 ^ GHorsePlayer];

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

	if (m250) {
		m250 -= 2;

		if (m250 <= 0)
			m250 = 0;
	}

	if (m254) {
		m254 -= 2;

		if (m254 <= 0) {
			m254 = 0;
			FUN_80042700(this, 0, 0, 0, 5);
		}
	}

	if ((mPad->Up.Pressed || mAY < -40) && !mPad->Left.Pressed && !mPad->Right.Pressed && !mAX) {
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
			ticks = m330 - XFrames;

		if (ticks < 0 && !GState) {
			if (GAutoTest)
				EndCode = 8;
			else {
				if (GGame == 1 && Levels[GLevel].f0C)
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
		m1A0 = 0;

	mKick.vx = mKick.vy = mKick.vz = 0;
	HandleKick();
	HandleJump();

	if (mPhysicsState != 5 && m14F) {
		switch (mAnim) {
			case 35:
			case 36:
				m150 = 0x10000;

				if (!mPhysicsState)
					Debug_RunAnim__6CBruceii(this, 0, 7869);
				else
					Debug_RunAnim__6CBruceii(this, 14, 7871);

				break;
		}
	}

	HandleTricksNew();

	/* HandleBraking */
	{
		int DownOnly = 0;

		if ((mPad->Down.Pressed && !mPad->Left.Pressed && !mPad->Right.Pressed) ||
		    (mAY > 0 && !mAX))
			DownOnly = 1;

		if (mPhysicsState == 7 && !DownOnly)
			StateTransition(0, 7888);

		if (!mPhysicsState && DownOnly && !m358) {
			int Angle = -(mNormal.vy << 12) >> 12;
			int BrakeVal;

			if (Angle < 768)
				Angle = 768;

			BrakeVal = 4096 - Angle;
			BrakeVal = BrakeVal * 6144 / 208;

			if (BrakeVal < 0xa000)
				BrakeVal = 0xa000;

			if (BrakeVal < FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6) {
				if (mAnim != 49) {
					m150 = 0x20000;
					SFX_PlayX(2, 0x2000, 0);
					Debug_RunAnim__6CBruceii(this, 49, 7923);
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
				StateTransition(7, 7943);
				mVel.Set(0, 0, 0);

				if (mAnim == 49)
					Debug_RunAnim__6CBruceii(this, 0, 7984);
			}

			m840 = 2;
		} else if (mAnim == 49) {
			m150 = 0x10000;
			Debug_RunAnim__6CBruceii(this, 0, 7996);
		}
	}

	if (m14F) {
		switch (mAnim) {
			case 11:
			case 12:
				if (mManIsCrouched) {
					m150 = 0x10000;
					Debug_RunAnim3__6CBruceiiiii(this, 8, 19, 26, 19, 8012);
				} else {
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 0, 8017);
				}

				break;
		}
	}

	m738 = mPhysicsState;
	mOldRotMatrix = mRotMatrix;
	m1B0 = mOldPos;
	mOldPos = mPos;
	m814.Set(0, 0, 0);
	HandleContinuousSounds();

	if (mManIsCrouched)
		mKickSpeed = MAXSPEED(22);

	if (m33C) {
		int NewMag = (FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6) >> 12;

		mKick.vx += mVel.vx / NewMag;
		mKick.vy += mVel.vy / NewMag;
		mKick.vz += mVel.vz / NewMag;
		m33C--;
	}

	if (m334 < FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6)
		m304 = 1;
	else
		m304 = 0;

	if (m340 > 0)
		m340--;
	else if (m344 > 0)
		m344--;
	else {
		m344 = 30;

		if (MAXSPEED(39) < m334)
			m334 -= 4096;

		if (MAXSPEED(17) < mKickSpeed)
			mKickSpeed -= 4096;
	}

	UberFrig(this, mPos, s_restart_at_start_of_main_physics);

	m1BC = mPos;

	if (!m358)
		BoardOn__6CBruce(this);

	if (mPhysicsState != 8)
		HandleStickToRail();

	if (m358 && mAnim == 46 && mFrame == HeadBloodAtFrame[mSkaterIndex])
		BloodSplat(0, 50);

	/* HandlePhysicsStates */
	mRailTime++;

	switch (mPhysicsState) {
		case 0:
			mBalance = 0;
			mBalanceVel = 0;
			m8EC = 0;
			mRailTime = 0;
			m20C = 0;
			m2DC = 0;
			m2E0 = 0;
			m1DC = mPos;
			DoOnGroundPhysics__6CBruce(this);
			break;
		case 7:
			DoOnGroundPhysics__6CBruce(this);

			mPos = mOldPos;
			break;
		case 4: {
			int i;

			if (m2E0) {
				Trick_Trick(77, m2E0->pName, -m2E0->Score, 0);
				SFX_PlayX(161, 0x2000, 0);

				m2E0 = 0;
			}

			for (i = 0; i < 10; i++)
				if (m2B4[i] < 10000)
					m28C[i] = 12345;

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
				StateTransition(1, 8263);

			break;
		case 2:
			DoOnGroundPhysics__6CBruce(this);
			break;
	}

	/* HandleBailStates */
	if (m358) {
		mKick.vx = 0;
		mKick.vz = 0;
		m744 = 0;
		m740 = 0;

		if ((!m19C || mPhysicsState == 2) && mVel.vy < 0)
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

				if (m14F && !mPhysicsState) {
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

				if (m14F && !mPhysicsState) {
					if (mAnim != 46) {
						Debug_RunAnim__6CBruceii(this, 55, 8366);

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

				if (m14F && !mPhysicsState) {
					Debug_RunAnim1__6CBruceiii(this, 54, 99, 8381);

					m24C = 4;
				}

				break;
			case 2:
				if (!mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 5) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 5) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 5) * TimeScale) >> 8;
				}

				m150 = 0x10000;
				Debug_RunAnim1__6CBruceiii(this, 54, 30, 8399);

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

				if (m14F && !mPhysicsState)
					m24C = 4;

				break;
			case 3:
				if (mFrame >= 51 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (m14F && !mPhysicsState)
					m24C = 4;

				break;
			case 8:
				if (mFrame < 18) {
					mKick.vy = 0;
					mVel.vx -= ((mVel.vx >> 4) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 4) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 4) * TimeScale) >> 8;
				}

				if (m14F && !mPhysicsState) {
					Debug_RunAnim1__6CBruceiii(this, 54, 76, 8451);

					m24C = 4;
				}

				break;
			case 9:
				if (mFrame >= 29 && !mPhysicsState) {
					mVel.vx -= ((mVel.vx >> 2) * TimeScale) >> 8;
					mVel.vy -= ((mVel.vy >> 2) * TimeScale) >> 8;
					mVel.vz -= ((mVel.vz >> 2) * TimeScale) >> 8;
				}

				if (m14F && !mPhysicsState) {
					Debug_RunAnim1__6CBruceiii(this, 46, 36, 8467);

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
					BoardOn__6CBruce(this);

				if (m14F) {
					m840 = 4;
					m150 = 0x10000;

					if (mManIsCrouched && mPad->Square.Pressed)
						Debug_RunAnim3__6CBruceiiiii(this, 8, 0, 26, 19, 8490);
					else {
						mManIsCrouched = 0;
						Debug_RunAnim__6CBruceii(this, 0, 8495);
					}

					m358 = 0;
					mVel.vx = mVel.vy = mVel.vz = 0;
					BoardOn__6CBruce(this);
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

		LInfo.StartCoords.Set(old.vx, old.vy, old.vz);
		LInfo.EndCoords.Set(mPos.vx, mPos.vy, mPos.vz);
		M3dColij_InitLineInfo(&LInfo);

		LInfo.Flag88 = 1;
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

	mVel += (mKick * TimeScale) >> 8;

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
