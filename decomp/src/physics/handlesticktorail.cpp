/* HandleStickToRail__6CBruce (0x8003058c) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Called from DoPhysics where the demo calls HandleStickToRail. THPS1 keeps the
   demo's StartHandPlant and StartGrind as inline helpers (their Debug_RunAnim lines 1536..1548 and
   1667..1835 are this file's __LINE__s) and adds a button-combo rail-trick lookup in the 44-byte
   trick table (entries from 32 on, keyed by style + two button codes) whose result both helpers use.
   Levers: the balance product distributes into GetStat's three arms only because the multiplicand
   is an inline call (GetBalanceVel(); a plain member or local is not distributed by fold); the
   table walk reads the body through a local table pointer and the test into a variable, which
   keeps the two pointer givs apart as in retail; `char *pName` loaded before the Trick_Trick call
   gives retail's score-first argument loads; the Limit test is two ifs on a State local (not an
   || that folds to & 0x6000); `case 0: break;` gives the m210 slti tree; Bailing as two `= 1`
   stores. StartGrind's Score is one function-scope local assigned in both Trick_Trick arms (set
   twice, so no sched1 set-once boost; as in FUN_8003159c) and read after the mGrindTrick store.
   Residues: the trick walk and the GetBalanceVel copy/arm register roles (a1/a2/a3: retail's DAT
   arm uses the load, the stat arms the copy; ours the reverse = cse1 swapped `321=mem; 322=321`
   into `322=mem; 321=322` because 322 outlives the block).
   Trick walk: retail's zero-trip `blez` goes straight to `RailTrick = 0` (jump-threaded past the
   post-loop recheck). The test written WITHOUT `f` (`Group[RailTrick].Flags > 0`) threads
   it, after which gcse PRE copies 2*RailTrick in both body and test and loop reduces the 2R giv
   exactly as retail (li 64 / +2), but still 2 words long: retail's entry load and loop test share
   one pseudo (t0; unboosted, so the entry lw lands early), and ours are two (renamed by
   duplicate_loop_exit_test), which also moves the 2R biv out of v0. With `f`, no threading. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern STrick Group[];
extern ushort DAT_800a7edc[];
extern char *DAT_800a7ef0[];

int Cheat_StatsOverride;

extern int Cheat_Fire;
extern int XFrames;

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])

int SFX_PlayX(unsigned int, short, int);
int Redbook_Rnd(int);

extern "C" {
	void FUN_8002f1a0(CBruce *);
	void Bail__6CBrucei13ELimitBailVel(CBruce *, int);
	void FUN_8003159c(CBruce *);
	int Mips_CVecDotProd__FP7CVectorT0(CVector *, CVector *);
}

inline void CBruce::StartHandPlant(int RailTrick, int Flags) {
	mAnimSpeed = 0x10000;
	m248 = 0;

	if (!m760) {
		FUN_8002f1a0(this);

		if (!m760) {
			mStickToRail = 0;
			return;
		}
	}

	if (RailTrick && (Flags & 0x2000000)) {
		char *pName = Group[RailTrick].pName;

		Trick_Trick(RailTrick, pName, Group[RailTrick].Score, 0);
		Debug_RunAnim(Group[RailTrick].Anim, 1536);

		m368 |= 0x4800;
		SFX_PlayX(162, 0x2000, 0);
	} else {
		Trick_Trick(79, "Handplant", 1000, 0);
		Debug_RunAnim1(34, 8, 1544);

		m368 |= 0x4800;
	}

	StateTransition(PHYSICS_IN_HANDPLANT, 1548);
	SFX_PlayX(38, 0x2000, 0);

	mPos = mRailPos;
	mPos.vy -= 30;
	TrickObRail();

	mVel.vx = 0;
	mVel.vz = 0;
	m710 = mRailStart;
	mVel.vy = -mVel.vy;
}

inline void CBruce::StartGrind(int RailTrick) {
	int RightDot;
	int Direction;
	int Score;

	if (!RailTrick || (Group[RailTrick].Flags & 0x2000000))
		RailTrick = 0;

	FlipToVelocity();

	m204 = 0;
	mPos = mRailPos;
	mStickToRail = 0;
	m6DC = 1;
	mPos.vy += -0x19000;
	m710 = mRailStart;
	mLastScorePos = mPos;
	m8CC = mPos;

	if (mRailStart != m6E4) {
		mRailTime -= 30;

		if (mRailTime < 0)
			mRailTime = 0;

		mBalanceVel = 0;
		m6E4 = mRailStart;
	} else
		mRailTime += 40;

	mBalanceVel = GetBalanceVel() * (100 - GetStat(3) * 15) / 100 * (mRailTime + 60) / 80;
	mGrinds++;

	{
		CVector Vel = mRailDir;

		Vel *= GetStat(1) + 5;
		mVel += Vel;
	}

	switch (m210) {
		case 0:
			break;
		case 1:
			SFX_PlayX(Redbook_Rnd(2) + 36, 0x2000, 0);
			break;
		case 2:
			SFX_PlayX(Redbook_Rnd(2) + 38, 0x2000, 0);
			break;
	}

	StateTransition(PHYSICS_ON_RAIL, 1667);

	mDoingATrick = 1;
	m424++;
	mRails++;
	TrickObRail();
	Mips_CVecDotProd__FP7CVectorT0(&mRailDir, &mFront);

	RightDot = Mips_CVecDotProd__FP7CVectorT0(&mRailDir, &mRight);
	m320 = 0;

	if (RailTrick) {
		SFX_PlayX(162, 0x2000, 0);

		m368 = 0x80001000;
		Score = Group[RailTrick].Score;
		Trick_Trick(RailTrick, Group[RailTrick].pName, Score, 0);

		m364 = 1;
		m2EC = 0;
		mGrindTrick = 0;

		if (!Group[RailTrick].Frames)
			Debug_RunAnim(Group[RailTrick].Anim, 1701);
		else
			Debug_RunAnim2(Group[RailTrick].Anim, 0, Group[RailTrick].Frames, 1705);

		return;
	}

	if (ABS(RightDot) < 2867) {
		SControl *pPad = mpPad;

		Direction = 6;

		if (pPad->Down.Pressed || mAY > 40) {
			if (pPad->Left.Pressed || mAX < -40) {
				Direction = 7;
				m320 = -180;
			} else if (pPad->Right.Pressed || mAX > 40) {
				Direction = 7;
				m320 = 180;
			} else
				Direction = 4;
		}

		if (pPad->Up.Pressed || mAY < -40) {
			if (pPad->Left.Pressed || mAX < -40)
				Direction = 8;
			else if (pPad->Right.Pressed || mAX > 40)
				Direction = 8;
			else
				Direction = 5;
		}
	} else {
		if (mExtraFlags & 2)
			RightDot = -RightDot;

		mExtraFlags &= ~2;

		if (RightDot < 0)
			Direction = 2;
		else
			Direction = 3;
	}

	m368 = 0x80001000;
	mGrindTrick = Direction;
	Score = DAT_800a7edc[Direction];
	Trick_Trick(Direction + 66, DAT_800a7ef0[Direction], Score, 0);

	m364 = 1;

	switch (Direction) {
		case 2:
			Debug_RunAnim2(17, 1, 8, 1808);

			m2EC = 2;
			break;
		case 3:
			Debug_RunAnim(17, 1813);

			m2EC = 1;
			break;
		case 4:
			Debug_RunAnim(67, 1817);

			m2EC = 0;
			break;
		case 5:
			Debug_RunAnim(68, 1823);

			m2EC = 0;
			break;
		case 6:
			Debug_RunAnim(70, 1827);

			m2EC = 0;
			break;
		case 8:
			Debug_RunAnim(57, 1831);

			m2EC = 0;
			break;
		case 7:
			Debug_RunAnim(58, 1835);

			m2EC = 0;
			break;
	}
}

void CBruce::HandleStickToRail() {
	int Bailing;
	int Limit;
	int RailTrick;
	int Flags;

	if (!mpPad->Grind.Pressed)
		m310 = 0;

	if (!mpPad->Grind.Pressed || mInAir || (mPhysicsState == 2 && mVel.vy > 0) || m314)
		goto NoRail;

	if (m70C > 0 && --m70C > 0) {
		m6DC = 0;
		goto NoRail;
	}

	if (!mStickToRail || m6DC)
		goto NoRail;

	m70C = 0;

	if (mPhysicsState == 0) {
		mNum180Spins = 0;
		mTallyAngles = 0;
	}

	Limit = 10000;

	{
		int State = m368;
		int Trick = State & 0xff;

		if (State & 0x4000)
			Limit = mNumFrames - Group[Trick].MinFrame;
		else if (State & 0x2000)
			Limit = mNumFrames - Group[Trick].MinFrame;
	}

	Bailing = 0;

	if (m720) {
		if (m728 > 0) {
			if ((m724 & 4095) < 3796)
				Bailing = 1;
		} else if ((m724 & 4095) > 300)
			Bailing = 1;
	}

	if ((mDoingATrick && !m360 && mFrame < Limit) || Bailing) {
		Bail__6CBrucei13ELimitBailVel(this, 0);

		mTrickBails++;
		return;
	}

	if (mDoingATrick) {
		FUN_8003159c(this);

		m364 = 1;
	}

	RailTrick = 0;
	Flags = 0;

	if (mBalanceFlash && m278 && XFrames - m274 < 35 && m268 && XFrames - m26C < 45) {
		STrick *pTab = Group;

		Flags = Stats[mSkaterIndex].ComboID + (m268 << 8) + (m278 << 4) + 3;
		int f;

		for (RailTrick = 32; (f = Group[RailTrick].Flags) > 0; RailTrick++) {
			if ((pTab[RailTrick].Flags & 0xfffff) == Flags) {
				Flags = pTab[RailTrick].Flags;
				break;
			}
		}

		if (Group[RailTrick].Flags <= 0)
			RailTrick = 0;
	}

	if (mVel.vy < 0 && ABS(mUp.vy) < 3000 && ABS(mRight.vy) < 2048 && mpPad->Up.Pressed)
		StartHandPlant(RailTrick, Flags);
	else if (m760)
		StartGrind(RailTrick);
	else
		goto NoRail;

	return;
NoRail:
	mStickToRail = 0;
}
