/* DoOnRailPhysics__6CBruce (0x800331e4) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin (fuzzy), locals from its MAIN.SYM. Grinding: settles the grind
   anims, vibrates the pad, throws sparks, scores the distance ground, then walks the skater along
   the rail's segments (stepping to the previous / next node, or falling off the rail where the
   rail ends or bends too sharply), lines the heading up with the rail and runs the balance meter
   (pad left/right nudges it, otherwise it wobbles randomly; too far off and the skater bails),
   finally tilting the rotation matrix by the balance.
   Levers: the anim-range test is a switch (signed slti compare tree, not a folded range check);
   Diff is a named local so its slot stays live and Gravity (declared after it, C++) lands above
   the spark locals; the loop-hoisted constant 1 shared by the first two fall-off copies is
   spilled and rematerialised, while the next-node branch has its own OffRail local (retail's s1);
   ABS(vy) > ABS(vx) loads vy first; the two non-zero RotAngle arms share one ?: store tail;
   Pos.vx = Pos.vz = 0 gives the vz, vx, vy store order. */
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
	int Length() const;
	CVector operator-() const;
};

struct CSVector {
	short vx, vy, vz;
	void Set(short x, short y, short z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

CVector operator*(const CVector &, const int &);
CVector operator>>(const CVector &, const int &);
CVector operator/(const CVector &, const int &);
CVector operator+(const CVector &, const CVector &);
CVector operator-(const CVector &, const CVector &);

struct SStats {
	char pad0[0xc];
	int Style; /* 0x0c */
	char pad10[0x18 - 0x10];
	int Stats[10]; /* 0x18 */
	char pad40[340 - 0x40];
};

struct SColl {
	char pad0[0x80];
	uchar x80; /* 0x80 */
	char pad81[0x90 - 0x81];
	uchar x90; /* 0x90 */
};

extern SStats Stats[];

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int Cheat_L2KillsBaddies;
extern int TimeScale;
extern int TimeScaleSquared;
extern int BitCount;
extern int UseCacheForBits;
extern ushort DAT_800a7edc[];

class CBruce {
  public:
	char pad0[0x4];
	CVector mPos;     /* 0x4 */
	CSVector mAngles; /* 0x10 */
	char pad16[0x6c - 0x16];
	CVector mVel;  /* 0x6c */
	CVector mKick; /* 0x78 */
	char pad84[0x144 - 0x84];
	short mFrame; /* 0x144 */
	ushort mAnim; /* 0x146 */
	char pad148[0x14f - 0x148];
	uchar m14F; /* 0x14f */
	int m150;   /* 0x150 */
	char pad154[0x198 - 0x154];
	uchar *mpStatBonus; /* 0x198 */
	char pad19c[0x204 - 0x19c];
	int m204; /* 0x204 */
	char pad208[0x210 - 0x208];
	int m210; /* 0x210 */
	char pad214[0x224 - 0x214];
	int m224; /* 0x224 */
	char pad228[0x254 - 0x228];
	int m254; /* 0x254 */
	char pad258[0x2e8 - 0x258];
	int mSkaterIndex; /* 0x2e8 */
	int m2EC;         /* 0x2ec */
	char pad2f0[0x320 - 0x2f0];
	int m320; /* 0x320 */
	char pad324[0x35c - 0x324];
	int m35C; /* 0x35c */
	char pad360[0x6dc - 0x360];
	int m6DC; /* 0x6dc */
	int m6E0; /* 0x6e0 */
	char pad6e4[0x6e8 - 0x6e4];
	int mRailStart; /* 0x6e8 */
	int mRailNode;  /* 0x6ec */
	char pad6f0[0x700 - 0x6f0];
	CVector mRailDir; /* 0x700 */
	int m70C;         /* 0x70c */
	char pad710[0x740 - 0x710];
	int m740; /* 0x740 */
	char pad744[0x78c - 0x744];
	MATRIX mRotMatrix; /* 0x78c */
	char pad7ac[0x7d8 - 0x7ac];
	CVector mFront; /* 0x7d8 */
	CVector mRight; /* 0x7e4 */
	char pad7f0[0x86c - 0x7f0];
	SColl *mpColl; /* 0x86c */
	char pad870[0x894 - 0x870];
	signed char m894; /* 0x894 */
	char pad895[0x8d8 - 0x895];
	CVector mLastScorePos; /* 0x8d8 */
	int mBalance;          /* 0x8e4 */
	int mBalanceVel;       /* 0x8e8 */
	char pad8ec[0x8f0 - 0x8ec];
	int mRailTime;   /* 0x8f0 */
	int mGrindTrick; /* 0x8f4 */
	void StateTransition(int, int);
	void Vibrate(int, int, int);
	void SetAnglesFromHeading();
	void AlignHeadingOnRail();
	void RotateHeadingAndVelocity(int, int, int);
	void RotMatrixFromFRUVectors();
	void FRUVectorsFromRotMatrix();
	void DoOnRailPhysics();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (Stats[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

short PingPong(int, int, int);
int Redbook_Rnd(int);
void Bit_SetSparkTrajectory(const CSVector &);
ushort *Trig_GetLinksPointer(int);
void Trig_GetPosition(CVector &, int);
void M3dUtils_NormaliseVector(CVector *);

extern "C" {
	void Bit_SetSparkTrajectoryCone__FRC8CSVector(CSVector *);
	void *__nw__4CBitUi(int);
	void __11CGrindSparkP5CBodyR7CVectoriii(void *, CBruce *, CVector *, int, int, int);
	int FUN_8002f290(CBruce *, int, int);
	void FUN_8002f438(CBruce *, int);
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void Debug_RunAnim1__6CBruceiii(CBruce *, int, int, int);
	void Bail__6CBrucei13ELimitBailVel(CBruce *, int);
	int FUN_8004f578(ushort *);
	int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);
	int Mips_FixMul__Fii(int, int);
	void SFX_PlayX__FUisi(int, int, int);
	MATRIX *FUN_80093240(SVECTOR *, MATRIX *);
	MATRIX *FUN_80093130(MATRIX *, MATRIX *);
}

void CBruce::DoOnRailPhysics() {
	CVector SegStart;
	CVector SegEnd;
	CVector SegDir;
	int Speed;
	int LastSpeed;
	int RemainingSpeed;
	int NextSegment;
	int Length;
	int t;
	int MajorAxis;
	int FirstTimeThroughLoop = 1;

	if (m14F) {
		switch (mAnim) {
			case 41:
			case 68:
				if (Stats[mSkaterIndex].Style == 0 ||
				    Stats[mSkaterIndex].Style == 0x6000)
					Debug_RunAnim__6CBruceii(this, 41, 3104);
				else
					Debug_RunAnim1__6CBruceiii(this, 41, 5, 3106);

				break;
			case 40:
			case 67:
				if (Stats[mSkaterIndex].Style == 0 ||
				    Stats[mSkaterIndex].Style == 0x6000)
					Debug_RunAnim__6CBruceii(this, 40, 3112);
				else
					Debug_RunAnim1__6CBruceiii(this, 40, 3, 3114);

				break;
			case 70:
				Debug_RunAnim__6CBruceii(this, 42, 3120);
				break;
			case 17:
				if (mFrame >= 16)
					Debug_RunAnim__6CBruceii(this, 39, 3125);
				else
					Debug_RunAnim__6CBruceii(this, 37, 3129);

				break;
			case 37:
				Debug_RunAnim__6CBruceii(this, 38, 3133);
				break;
			case 57:
				mFrame = PingPong(8, 10, 15);
				break;
			case 58:
				mFrame = PingPong(6, 8, 15);
				break;
			case 62:
			case 63:
			case 64:
				switch (Stats[mSkaterIndex].Style) {
					case 0x2000:
						mFrame = PingPong(23, 25, 15);
						break;
					case 0x4000:
						mFrame = PingPong(6, 18, 20);
						break;
					case 0x5000:
						mFrame = PingPong(14, 16, 15);
						break;
					case 0x6000:
						mFrame = PingPong(21, 23, 15);
						break;
					case 0x8000:
						mFrame = PingPong(10, 12, 15);
						break;
				}

				break;
			default:
				Debug_RunAnim__6CBruceii(this, mAnim, 3179);
				break;
		}
	}

	Vibrate(2, 0, 1);

	if (!m2EC) {
		switch (mAnim) {
			case 62:
			case 63:
			case 64:
				switch (Stats[mSkaterIndex].Style) {
					case 0x2000:
					case 0x5000:
					case 0x6000:
						goto NoSparks;
				}
		}

		if (BitCount < 200) {
			SetAnglesFromHeading();
			Bit_SetSparkTrajectory(mAngles);

			{
				CSVector Cone;
				CVector Pos;
				int i;

				Cone.Set(450, 900, 0);
				Bit_SetSparkTrajectoryCone__FRC8CSVector(&Cone);

				UseCacheForBits = 0;
				Pos.vx = Pos.vz = 0;
				Pos.vy = 0x19000;

				for (i = 0; i < 9; i++) {
					void *p = __nw__4CBitUi(120);

					if (p)
						__11CGrindSparkP5CBodyR7CVectoriii(p, this, &Pos, -40, 4096, 5);
				}

				UseCacheForBits = 1;
			}
		}
	}
NoSparks:
	mKick.Set(0, 0, 0);
	CVector Diff = mPos - mLastScorePos;

	if (Diff.Length() > 30) {
		FUN_8002f438(this, FUN_8002f290(this, mGrindTrick + 66, DAT_800a7edc[mGrindTrick]) / 5);

		mLastScorePos = mPos;
	}

	CVector Gravity;

	Gravity.Set(0, 13000, 0);

	do {
		int AccMagnitude;

		NextSegment = FUN_8004f578(Trig_GetLinksPointer(mRailNode));
		Trig_GetPosition(SegStart, mRailNode);
		Trig_GetPosition(SegEnd, NextSegment);

		SegDir = SegEnd - SegStart;
		M3dUtils_NormaliseVector(&SegDir);

		Speed = Mips_CVecDotProd__FP7CVectorT0(&SegDir, &mVel);

		if (FirstTimeThroughLoop) {
			FirstTimeThroughLoop = 0;
			mKick += Gravity;

			if (!Speed)
				Speed = 1000;

			if (ABS(Speed) < 60000) {
				if (SegDir.vy > 0)
					Speed += 2000;
				else if (SegDir.vy < 0)
					Speed -= 2000;
				else
					Speed = Speed * 5 / 4;
			}
		}

		AccMagnitude = Mips_CVecDotProd__FP7CVectorT0(&SegDir, &mKick);
		mKick.vx = Mips_FixMul__Fii(SegDir.vx, AccMagnitude);
		mKick.vy = Mips_FixMul__Fii(SegDir.vy, AccMagnitude);
		mKick.vz = Mips_FixMul__Fii(SegDir.vz, AccMagnitude);
		mVel.vx = Mips_FixMul__Fii(SegDir.vx, Speed);
		mVel.vy = Mips_FixMul__Fii(SegDir.vy, Speed);
		mVel.vz = Mips_FixMul__Fii(SegDir.vz, Speed);
		MajorAxis = 0;

		if (ABS(SegDir.vy) > ABS(SegDir.vx)) {
			MajorAxis = 1;

			if (ABS(SegDir.vy) < ABS(SegDir.vz))
				MajorAxis = 2;
		} else if (ABS(SegDir.vx) < ABS(SegDir.vz))
			MajorAxis = 2;

		switch (MajorAxis) {
			case 0:
				Length = ((SegEnd.vx - SegStart.vx) / SegDir.vx) << 12;
				t = ((mPos.vx - SegStart.vx) / SegDir.vx) << 12;
				break;
			case 1:
				Length = ((SegEnd.vy - SegStart.vy) / SegDir.vy) << 12;
				t = ((mPos.vy - SegStart.vy) / SegDir.vy) << 12;
				break;
			case 2:
				Length = ((SegEnd.vz - SegStart.vz) / SegDir.vz) << 12;
				t = ((mPos.vz - SegStart.vz) / SegDir.vz) << 12;
				break;
		}

		t += Speed;
		LastSpeed = Speed;

		if (t >= 0 && t <= Length) {
			mPos += ((mVel * TimeScale) >> 8) + ((mKick * TimeScaleSquared) >> 8) / 2;
			RemainingSpeed = 0;
		} else if (t < 0) {
			int PreviousSegment = -1;
			int n;

			for (n = mRailStart; n != mRailNode; n = FUN_8004f578(Trig_GetLinksPointer(n)))
				PreviousSegment = n;

			if (PreviousSegment == -1) {
				StateTransition(1, 3402);

				m70C = 5;
				m150 = 0x10000;
				Debug_RunAnim__6CBruceii(this, 14, 3405);

				RemainingSpeed = 0;
				m6DC = 0;
				m6E0 = 0;
				m740 = 1;
				m35C = 0;

				if (m210 == 1 && !m2EC)
					SFX_PlayX__FUisi(16, 0x2000, 0);
			} else {
				int NextSegment2 = FUN_8004f578(Trig_GetLinksPointer(PreviousSegment));
				CVector SegStart2;
				CVector SegEnd2;
				CVector SegDir2;

				Trig_GetPosition(SegStart2, PreviousSegment);
				Trig_GetPosition(SegEnd2, NextSegment2);

				SegDir2 = SegEnd2 - SegStart2;
				M3dUtils_NormaliseVector(&SegDir2);

				if (Mips_CVecDotProd__FP7CVectorT0(&SegDir, &SegDir2) < 2633) {
					StateTransition(1, 3440);

					m70C = 5;
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 14, 3443);

					RemainingSpeed = 0;
					m6DC = 0;
					m6E0 = 0;
					m740 = 1;
					m35C = 0;

					if (m210 == 1 && !m2EC)
						SFX_PlayX__FUisi(16, 0x2000, 0);
				} else {
					RemainingSpeed = t;
					mRailNode = PreviousSegment;
					mPos = SegStart;
					mPos.vy -= 0x19000;
				}
			}
		} else {
			int OffRail = 1;

			if (!*Trig_GetLinksPointer(NextSegment)) {
				StateTransition(1, 3477);

				m70C = 5;
				m150 = 0x10000;
				Debug_RunAnim__6CBruceii(this, 14, 3480);

				RemainingSpeed = 0;
				m6DC = 0;
				m6E0 = 0;
				m740 = OffRail;
				m35C = 0;

				if (m210 == 1 && !m2EC)
					SFX_PlayX__FUisi(16, 0x2000, 0);
			} else {
				int NextSegment2 = FUN_8004f578(Trig_GetLinksPointer(NextSegment));
				CVector SegStart2;
				CVector SegEnd2;
				CVector SegDir2;

				Trig_GetPosition(SegStart2, NextSegment);
				Trig_GetPosition(SegEnd2, NextSegment2);

				SegDir2 = SegEnd2 - SegStart2;
				M3dUtils_NormaliseVector(&SegDir2);

				if (Mips_CVecDotProd__FP7CVectorT0(&SegDir, &SegDir2) < 2633) {
					StateTransition(1, 3517);

					m70C = 5;
					m150 = 0x10000;
					Debug_RunAnim__6CBruceii(this, 14, 3520);

					RemainingSpeed = 0;
					m6DC = 0;
					m6E0 = 0;
					m740 = OffRail;
					m35C = 0;

					if (m210 == 1 && !m2EC)
						SFX_PlayX__FUisi(16, 0x2000, 0);
				} else {
					mRailNode = NextSegment;
					RemainingSpeed = t - Length;
					mPos = SegEnd;
					mPos.vy -= 0x19000;
				}
			}
		}
	} while (RemainingSpeed);

	if (LastSpeed >= 0) {
		mRailDir = -SegDir;

		if (m2EC) {
			mFront.vx = -SegDir.vz;
			mFront.vy = -SegDir.vy;
			mFront.vz = SegDir.vx;
		} else {
			mFront.vx = -SegDir.vx;
			mFront.vy = -SegDir.vy;
			mFront.vz = -SegDir.vz;
		}
	} else {
		mRailDir = SegDir;

		if (m2EC) {
			mFront.vx = SegDir.vz;
			mFront.vy = SegDir.vy;
			mFront.vz = -SegDir.vx;
		} else {
			mFront.vx = SegDir.vx;
			mFront.vy = SegDir.vy;
			mFront.vz = SegDir.vz;
		}
	}

	if (!m6DC) {
		if (LastSpeed >= 0)
			mFront = -SegDir;
		else
			mFront = SegDir;
	}

	AlignHeadingOnRail();
	RotateHeadingAndVelocity(m320, 0, 0);

	if (m2EC == 2) {
		mFront = -mFront;
		mRight = -mRight;
		RotMatrixFromFRUVectors();
	}

	mBalance += mBalance / (GetStat(3) * 3 + 28) * (mRailTime + 60) / 80;
	mBalance += mBalanceVel * (mRailTime + 60) / 80;

	if (Cheat_L2KillsBaddies)
		mBalance = 0;

	if (!mpColl->x80 && m894 >= -40 && !mpColl->x90 && m894 <= 40)
		m204 = 1;

	if ((mpColl->x80 || m894 < -40) && m204)
		mBalanceVel -= 20;
	else if ((mpColl->x90 || m894 > 40) && m204)
		mBalanceVel += 20;
	else if (ABS(mBalanceVel) < 20 - GetStat(3))
		mBalanceVel = Redbook_Rnd(80 - GetStat(3) * 3) - (80 - GetStat(3) * 3) / 2;

	if (ABS(mBalance) > 4000) {
		Bail__6CBrucei13ELimitBailVel(this, 0);

		m224++;
	}

	{
		MATRIX Rail_RotMatrix;
		SVECTOR RotAngle;

		RotAngle.vy = 0;

		if (!m2EC) {
			RotAngle.vx = 0;
			RotAngle.vz = (mBalance / 10) & 4095;
		} else {
			RotAngle.vx = (m2EC == 1 ? -(mBalance / 10) : mBalance / 10) & 4095;
			RotAngle.vz = 0;
		}

		FUN_80093240(&RotAngle, &Rail_RotMatrix);
		FUN_80093130(&mRotMatrix, &Rail_RotMatrix);
		FRUVectorsFromRotMatrix();
	}
}
