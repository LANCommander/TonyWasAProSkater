/* HandleJump__6CBruce (0x80038268) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin, locals from its MAIN.SYM. Ollie handling: settles the crouch
   anims, plays the landing sound/vibration and scores the landing, counts the crouch (ollie
   charge) timer while on the ground, and on release launches the skater: off a rail, along the
   rail; off a wall-ride, away from the wall; the upward kick scales with the charge and the
   skater's stats. A quick double-tap within the window does a boneless / no-comply.
   GetStat is the shared stat macro (see dofootplant.cpp). Levers: SFX_PlayX (SFX_PlayX__FUisi) is
   declared void here (v0 free for the |mUp.vy| test); the two JumpBoost arms share their tail by
   cross-jumping, so the boost is a local added once; the early-out call is wrapped in
   do { } while (0) (keeps `sw m744` ahead of the argument setup). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

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

struct CVector {
	int vx, vy, vz;
	CVector &operator+=(const CVector &);
};

CVector operator*(const CVector &, const int &);

struct SSkater {
	char pad0[0x18];
	int Stats[10]; /* 0x18 */
	char pad40[340 - 0x40];
};

struct SColl {
	char pad0[0x30];
	unsigned char x30; /* 0x30 */
	char pad31[0x80 - 0x31];
	unsigned char x80; /* 0x80 */
	char pad81[0x90 - 0x81];
	unsigned char x90; /* 0x90 */
};

struct CPanelData {
	char pad[4];
};

extern SSkater DAT_800a44fc[];

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int XFrames;
extern int GState;
extern char s_FASTPLANT[];
extern char s_NOLLIE[];

class CBruce {
  public:
	char pad0[0x6c];
	CVector mVel;  /* 0x6c */
	CVector mKick; /* 0x78 */
	char pad84[0x144 - 0x84];
	short mFrame;         /* 0x144 */
	unsigned short mAnim; /* 0x146 */
	char pad148[0x14b - 0x148];
	signed char m14B; /* 0x14b */
	char pad14c[0x14f - 0x14c];
	unsigned char m14F; /* 0x14f */
	int m150;           /* 0x150 */
	char pad154[0x198 - 0x154];
	unsigned char *mpStatBonus; /* 0x198 */
	char pad19c[0x1e8 - 0x19c];
	int m1E8; /* 0x1e8 */
	char pad1ec[0x210 - 0x1ec];
	int m210; /* 0x210 */
	char pad214[0x228 - 0x214];
	int m228; /* 0x228 */
	int m22C; /* 0x22c */
	char pad230[0x254 - 0x230];
	int m254; /* 0x254 */
	char pad258[0x268 - 0x258];
	int m268; /* 0x268 */
	int m26C; /* 0x26c */
	char pad270[0x274 - 0x270];
	int m274; /* 0x274 */
	int m278; /* 0x278 */
	char pad27c[0x2e8 - 0x27c];
	int mSkaterIndex; /* 0x2e8 */
	int m2EC;         /* 0x2ec */
	char pad2f0[0x2f8 - 0x2f0];
	int m2F8; /* 0x2f8 */
	int m2FC; /* 0x2fc */
	int m300; /* 0x300 */
	char pad304[0x310 - 0x304];
	int m310; /* 0x310 */
	char pad314[0x328 - 0x314];
	int m328; /* 0x328 */
	int m32C; /* 0x32c */
	char pad330[0x33c - 0x330];
	int m33C; /* 0x33c */
	int m340; /* 0x340 */
	int m344; /* 0x344 */
	char pad348[0x358 - 0x348];
	int mInAir; /* 0x358 */
	int m35C;   /* 0x35c */
	char pad360[0x364 - 0x360];
	int m364; /* 0x364 */
	int m368; /* 0x368 */
	char pad36c[0x6dc - 0x36c];
	int m6DC; /* 0x6dc */
	int m6E0; /* 0x6e0 */
	char pad6e4[0x700 - 0x6e4];
	CVector mRailDir; /* 0x700 */
	int m70C;         /* 0x70c */
	char pad710[0x72c - 0x710];
	int m72C; /* 0x72c */
	char pad730[0x734 - 0x730];
	int m734; /* 0x734 */
	int m738; /* 0x738 */
	int m73C; /* 0x73c */
	int m740; /* 0x740 */
	int m744; /* 0x744 */
	int m748; /* 0x748 */
	int m74C; /* 0x74c */
	char pad750[0x754 - 0x750];
	int m754; /* 0x754 */
	char pad758[0x764 - 0x758];
	int m764; /* 0x764 */
	int m768; /* 0x768 */
	char pad76c[0x7d8 - 0x76c];
	CVector mFront; /* 0x7d8 */
	CVector mRight; /* 0x7e4 */
	CVector mUp;    /* 0x7f0 */
	char pad7fc[0x80c - 0x7fc];
	SVECTOR mPolyNormal; /* 0x80c */
	char pad814[0x838 - 0x814];
	int m838; /* 0x838 */
	char pad83c[0x86c - 0x83c];
	SColl *mpColl; /* 0x86c */
	char pad870[0x924 - 0x870];
	CPanelData mPanel; /* 0x924 */
	void Trick_Trick(int, char *, int, int);
	void StateTransition(int, int);
	void Vibrate(int, int, int);
	void AlignHeadingOnRail();
	void RotateHeadingAndVelocity(int, int, int);
	void HandleJump();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (DAT_800a44fc[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

int Redbook_Rnd(int);

extern "C" {
	int Mips_CVecDotProd__FP7CVectorT0(void *, void *);
	int FUN_8008b5dc(int);
	int Mips_FixMul__Fii(int, int);
	void SFX_PlayX__FUisi(int, int, int);
	int FUN_8006ebdc(CPanelData *);
	void FUN_8002f1a0(CBruce *);
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void Debug_RunAnim1__6CBruceiii(CBruce *, int, int, int);
	void Debug_RunAnim2__6CBruceiiii(CBruce *, int, int, int, int);
	void Debug_RunAnim3__6CBruceiiiii(CBruce *, int, int, int, int, int);
	void HandleBailOnLanding__6CBrucei(CBruce *, int);
	void FUN_8002f458(CBruce *, int);
	void Trick_Land__6CBruce(CBruce *);
}

void CBruce::HandleJump() {
	int Speed;

	if (mInAir)
		return;

	if (m73C) {
		if (!mAnim) {
			m150 = 0x10000;
			Debug_RunAnim3__6CBruceiiiii(this, 8, 0, 26, 19, 6105);
		}
	} else if (mAnim == 8) {
		m150 = 0x10000;

		if (m14B == -1) {
			if (!mFrame)
				Debug_RunAnim__6CBruceii(this, 0, 6114);
		} else
			Debug_RunAnim2__6CBruceiiii(this, 8, 19, 0, 6117);
	}

	if (m14F) {
		switch (mAnim) {
			case 8:
				if (!mFrame) {
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 0, 6130);
				}

				break;
			case 5:
			case 25:
				if (m73C) {
					m150 = 0x10000;
					Debug_RunAnim3__6CBruceiiiii(this, 8, 19, 26, 19, 6145);
				} else {
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 0, 6150);
				}

				break;
			case 4:
			case 13:
				if (!m73C) {
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 14, 6160);
				}

				break;
			case 14:
				m150 = 0x10000;
				Debug_RunAnim__6CBruceii(this, 14, 6166);
				break;
			case 34:
				m150 = 0x10000;

				if (m73C)
					Debug_RunAnim3__6CBruceiiiii(this, 8, 19, 26, 19, 6175);
				else
					Debug_RunAnim2__6CBruceiiii(this, 8, 19, 0, 6178);

				break;
			case 0:
				m150 = 0x10000;

				if (m73C)
					Debug_RunAnim3__6CBruceiiiii(this, 8, 19, 26, 19, 6191);
				else
					Debug_RunAnim__6CBruceii(this, 0, 6194);

				break;
		}
	}

	if (m740 && m738 && !m734) {
		m740 = 0;
		m764 = 0;

		if (!mInAir) {
			switch (m72C) {
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

			if (m73C) {
				m150 = 0x10000;
				Debug_RunAnim1__6CBruceiii(this, 25, 3, 6253);
			} else {
				m150 = 0x10000;
				Debug_RunAnim1__6CBruceiii(this, 5, 0, 6258);
			}

			HandleBailOnLanding__6CBrucei(this, 6261);

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
				Trick_Land__6CBruce(this);
			}
		}

		m35C = 0;
	}

	if (mpColl->x30 && !mInAir) {
		m748++;

		if (m748 > 15 - (GetStat(0) + GetStat(2)) / 2)
			m748 = 15 - (GetStat(0) + GetStat(2)) / 2;

		if (m734 == 0 && !m310) {
			m744 = 1;

			if (!m73C) {
				m150 = 0x10000;

				if (ABS(m838) <= 0x9fff) {
					m838 = 0;
					Debug_RunAnim3__6CBruceiiiii(this, 8, 0, 26, 19, 6331);
				}
			}
		} else if (m734 == 4 && !m310) {
			m744 = 1;
		} else if (m734 == 5 && !m310) {
			m744 = 1;
		}

		if (!m310)
			m73C = 1;
	} else {
		m310 = 0;
		m73C = 0;

		if (m300 > 0 && XFrames - m300 > 20)
			m744 = 0;

		if (m744) {
			int LastPhysics;
			int JumpBoost;

			if (GState == 1 && m734 != 4 && m734 != 5) {
				m744 = 0;

				do {
					Debug_RunAnim__6CBruceii(this, 0, 6362);
				} while (0);

				m748 = 0;
				return;
			}

			m744 = 0;
			m740 = 1;
			m74C = m748;
			m2F8 = FUN_8008b5dc(Mips_CVecDotProd__FP7CVectorT0(&mVel, &mVel)) << 6;

			if (m6DC) {
				int a;

				m70C = 5;
				m6DC = 0;
				m6E0 = 0;
				m35C = 0;
				mFront = mRailDir;
				AlignHeadingOnRail();

				Speed = Mips_CVecDotProd__FP7CVectorT0(&mRailDir, &mVel);
				mVel.vx = Mips_FixMul__Fii(mRailDir.vx, Speed);
				mVel.vy = Mips_FixMul__Fii(mRailDir.vy, Speed);
				mVel.vz = Mips_FixMul__Fii(mRailDir.vz, Speed);
				a = 0;

				if (mpColl->x80)
					a = -200;

				if (mpColl->x90)
					a = 200;

				RotateHeadingAndVelocity(a, 1, 0);
			}

			m150 = 0x10000;
			m764 = m768;
			m754 = m734 == 5;
			LastPhysics = m734;

			if (LastPhysics == 5) {
				CVector SurfaceNormal;

				m748 = 15 - (GetStat(0) + GetStat(2)) / 2;
				SetVector(&SurfaceNormal, mPolyNormal.vx, mPolyNormal.vy, mPolyNormal.vz) mKick +=
				    SurfaceNormal * 2;
				mVel.vy -= 0xf000;
			}

			{
				int Sfx = 6;

				if (m734 == 4 && !m2EC) {
					Sfx = 16;

					if (m210 != 1)
						Sfx = 6;
				}

				SFX_PlayX__FUisi(Sfx, 0x2000, 0);
			}

			if (ABS(mUp.vy) < 2500)
				JumpBoost = -(
				    (m748 * (((32 - (10 - GetStat(2)) / 2) + ((10 - GetStat(2)) / 3 - 18)) << 10) /
					     (15 - (GetStat(0) + GetStat(2)) / 2) +
					 ((32 - (10 - GetStat(2)) / 2) << 10)) *
				    3);
			else
				JumpBoost = -(
				    (m748 * (((32 - (10 - GetStat(0)) / 2) + ((10 - GetStat(0)) / 3 - 18)) << 10) /
					     (15 - (GetStat(0) + GetStat(2)) / 2) +
					 ((32 - (10 - GetStat(0)) / 2) << 10)) *
				    3);

			mVel.vy += JumpBoost;
			m228++;

			if (m748 < 15 - (GetStat(0) + GetStat(2)) / 2)
				m22C++;

			m328 = 0;
			m32C = 0;

			if (m734 != 2) {
				if (m764 && m734 == 0) {
					StateTransition(3, 6480);

					m1E8 = XFrames;
				} else
					StateTransition(1, 6485);
			}

			if (!m368 || (m368 & 0xf000) == 0x1000) {
				m150 = 0x10000;

				if (LastPhysics != 5 && m278 == 1 && XFrames - m274 < 20) {
					m328 = 0;
					m32C = 0;

					if (m268 == 1 && XFrames - m26C < 35) {
						FUN_8002f1a0(this);
						Trick_Trick(65, s_FASTPLANT, 250, 0);

						mVel.vy -= 0x5000;
						Debug_RunAnim1__6CBruceiii(this, 45, 16, 6508);
					} else {
						FUN_8002f1a0(this);
						Trick_Trick(75, s_NOLLIE, 200, 0);
						Debug_RunAnim__6CBruceii(this, 13, 6515);
					}

					m364 = 1;
				} else {
					FUN_8002f1a0(this);
					Debug_RunAnim__6CBruceii(this, 4, 6523);
				}
			}

			m2FC = XFrames;
		}

		m748 = 0;
	}
}
