/* PHYSICS.CPP: the skater's physics: CBruce's movement states (on ground, in air, on rail,
   wall ride, foot/hand plants), tricks, spins, bails, jumps and kicks, skater collisions and
   continuous sounds (CBruce in game/bruce.h; the free functions in game/physics.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/physics.h"
#include "game/colij.h"
#include "game/vector.h"
#include "game/bruce.h"
#include "game/panel.h"
#include "game/trickob.h"
#include "game/crowd.h"
#include "game/mem.h"
#include "game/psyq.h"
#include "game/m3dasm.h"
#include "game/career.h"
#include "game/video.h"
#include "game/m3d.h"
#include "game/bit.h"
#include "game/sfx.h"
#include "game/main.h"
#include "game/bit2.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/m3dutils.h"
#include "game/m3dzone.h"
#include "game/platform.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/mech.h"
#include "game/rail.h"

extern int Face_Air;
extern int Face_Skateable;
extern int Face_WallRide;
extern int Face_Visible;
extern "C" {
	extern CVector DAT_800a7f34;
}
void ProjectCVectorToPlane(CVector *, SVECTOR);
#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
extern STrick Group[];
extern ushort DAT_800a7edc[];
extern char *DAT_800a7ef0[];
#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       mpStatBonus[n])
int InterpolateFrame(int, int);
extern int Cheat_L2KillsBaddies;
#define ST(d, b, q, t)                                                                             \
	q = (t);                                                                                       \
	d = b + (mInAir ? q * 35 : q * 5) * 2;
#define SetVector(v, x, y, z)                                                                      \
	{                                                                                              \
		int _x = (x);                                                                              \
		int _y = (y);                                                                              \
		int _z = (z);                                                                              \
		(v)->vx = _x;                                                                              \
		(v)->vy = _y;                                                                              \
		(v)->vz = _z;                                                                              \
	}
void CVectorRotateToPlane(CVector *, SVECTOR *, CVector *);
extern SStats DAT_800a7f40;
extern STrick DAT_800a8094, DAT_800a80c0, DAT_800a80ec;
extern CVector WorldUp;
int PingPong(int, int, int);
#define MaxSpeed (((GetStat(1) + 50) << 12) * 3 * 15 / 28)
extern STrick DAT_800a7250, DAT_800a727c, DAT_800a72a8, DAT_800a72d4, DAT_800a7300, DAT_800a732c;
extern short HeadBloodAtFrame[];
extern CVector DAT_800d25e4;
extern int DAT_800d1950;
#define MAXSPEED(k) (((GetStat(1) + (k)) << 12) * 3 * 15 / 28)
void UberFrig(CBruce *, CVector, char *);

/* GetFaceFlags__FP9SLineInfo (0x8002ee6c) */
void GetFaceFlags(SLineInfo *li) {
	if (li->pItem) {
		uint *f = li->pFace;
		uint fl = f[3];

		Face_Air = (fl >> 16) & 0x40;
		Face_Skateable = !(fl & 0x1000000);
		Face_WallRide = !(fl & 0x800000);
		Face_Visible = f[0] & 0x80;
		Face_Terrain = ((fl >> 16) & 0x1e00) >> 9;
	}
}

/* PingPong__Fiii (0x8002eed8) */
int PingPong(int lo, int hi, int speed) {
	int t = XFrames / (60 / speed);
	int range = hi - lo;
	int r = t % (range * 2);

	if (range < r)
		r = range * 2 - r;

	return lo + r;
}

/* SetGravity__FP7CVector (0x8002ef24) */
void SetGravity(CVector *v) {
	DAT_800a7f28 = *v;
	int len = -DAT_800a7f28.Length();

	DAT_800a7f34 = DAT_800a7f28 / len;
}

/* FUN_8002efa4 (0x8002efa4) */
/* C++. Before a skater is drawn: keeps its rotation and position (mTempRotMatrix, mTempPos) and,
   while it flips (m720), turns it by the flip angle about a point 0x46000 up its own y axis,
   moving it so that point stays put; then builds its transform. */
extern "C" void FUN_8002efa4(CBruce *p) {
	MATRIX m;
	SVECTOR ang;
	VECTOR a;
	VECTOR b;
	VECTOR c1;
	VECTOR c2;

	p->mTempRotMatrix = p->mRotMatrix;
	p->mTempPos = p->mPos;

	if (p->m720) {
		a.vz = 0;
		a.vy = 0;
		a.vx = 0;
		TransMatrix(&p->mRotMatrix, &a);
		TransMatrix(&m, &a);

		b.vx = 0;
		b.vy = 0x46000;
		b.vz = 0;
		ApplyMatrixLV(&p->mRotMatrix, &b, &c1);

		ang.vz = 0;
		ang.vy = 0;
		ang.vx = (ushort)p->m724 & 4095;
		RotMatrixYXZ(&ang, &m);
		MulMatrix(&p->mRotMatrix, &m);
		ApplyMatrixLV(&p->mRotMatrix, &b, &c2);

		p->mPos.vx = p->mPos.vx + c1.vx - c2.vx;
		p->mPos.vy = p->mPos.vy + c1.vy - c2.vy;
		p->mPos.vz = p->mPos.vz + c1.vz - c2.vz;
	}

	M3d_BuildTransform(p);
}

/* TempRestore__6CBruce (0x8002f118) */
/* Restores the rotation matrix and position saved by TempTransform. */
void CBruce::TempRestore() {
	mRotMatrix = mTempRotMatrix;
	mPos = mTempPos;
}

/* Trick_PlayerInit__6CBruceUs (0x8002f178) */
/* Initialises the panel (score display) for this player. */
void CBruce::Trick_PlayerInit(ushort Player) {
	Panel_PlayerInit(Player, this);
}

/* Trick_TakeOff__6CBruce (0x8002f1a0) */
/* Leaving the ground: unless wall-riding or out of time, starts the air
   (m760, m1FC = 15), resets the trick buffers (not when coming off an invisible surface), notes
   whether the wall item is a trick object (m34C), clears the spin score, resets the panel's
   combo (Panel_TakeOff) and the per-combo trick counts (and in game mode 2 the per-run ones). */
void CBruce::Trick_TakeOff() {
	int i;

	if (mPhysicsState == PHYSICS_IN_WALLRIDE)
		return;

	if (!mTime)
		return;

	m760 = 1;
	m200 = 0;
	m1FC = 15;

	if (mPhysicsState != PHYSICS_ON_INVISIBLE)
		ResetTrickBuffers();

	m34C = 0;

	if (mpWallItem) {
		if (Mem_CheckHandle(&mWallHandle))
			m34C = TrickOb_GetItemType(mpWallItem);
	}

	mNoSpinScore = 0;
	mNoSpin = 0;
	Panel_TakeOff(this, 0);

	for (i = 0; i < 80; i++)
		m568[i] = 0;

	if (GGame == 2) {
		int j;

		for (j = 0; j < 80; j++)
			m428[j] = 0;
	}
}

/* Trick_RepeatScore__6CBruceii (0x8002f290) */
/* Depreciates a trick's score by how often it has been done (the per-trick
   count m428[Trick]): 0-1 times full, 2 times 75%, 3 times 1/2, 4 times 1/4, otherwise 1/10. */
int CBruce::Trick_RepeatScore(int Trick, int Score) {
	switch (m428[Trick]) {
		case 0:
		case 1:
			break;
		case 2:
			Score = Score * 75 / 100;
			break;
		case 3:
			Score /= 2;
			break;
		case 4:
			Score /= 4;
			break;
		default:
			Score /= 10;
			break;
	}

	return Score;
}

/* Trick_Trick__6CBruceiPcii (0x8002f340) */
/* Scores a trick: counts it, depreciates its score by how often it was done (a
   negative score is taken as-is), zeroes it on a rail in some mode, and adds it to the panel. */
void CBruce::Trick_Trick(int Trick, char *pName, int Score, int) {
	if (m760) {
		if (GGame == 7)
			mTime = 0;

		m428[Trick]++;
		m568[Trick]++;

		if (Score < 0)
			Score = -Score;
		else
			Score = Trick_RepeatScore(Trick, Score);

		if (m34C == 4) {
			m428[Trick]--;
			Score = 0;
			m568[Trick]--;
		}

		Panel_Trick(this, pName, Score, 0);
	}
}

/* Trick_HoldTrick__6CBrucei (0x8002f438) */
/* Forwards to Panel_HoldTrick, which adds Score to the last queued trick in the
   skater's panel and refreshes the trick message. */
void CBruce::Trick_HoldTrick(int Score) {
	Panel_HoldTrick(this, Score);
}

/* Trick_Bonus__6CBrucei (0x8002f458) */
/* Forwards to Panel_Bonus, which is empty in retail. The arguments pass through
   in a0/a1 untouched, so the call is written like Trick_HoldTrick's. */
void CBruce::Trick_Bonus(int Score) {
	Panel_Bonus(this, Score);
}

/* Trick_Land__6CBruce (0x8002f478) */
/* Fuzzy demo twin Trick_Land (demo PHYSICS.CPP:605). The skater lands a trick:
   count the landing, score the spin as an "N OLLIE" (THPS1 names each spin amount where the demo
   computes -(n*50)), refresh the panel, fire the pending item, and feed the crowd meter into the
   special meter. */
void CBruce::Trick_Land() {
	mTricks++;
	m720 = 0;

	if (!mNoSpinScore) {
		int Spin = mTallyAngles;

		if (((Spin ^ (Spin >> 31)) - (Spin >> 31)) > 1500)
			mNum180Spins++;
	}

	if (mNum180Spins == 1 && mOldPhysicsState == 2)
		mNum180Spins = 0;

	mPanel.TrickMessageRefresh();

	mNoSpinScore = 0;
	mNoSpin = 0;

	if (!m364 && mNum180Spins) {
		if (mOldPhysicsState != 2 || mNum180Spins >= 2) {
			int n = mNum180Spins;

			mNum180Spins = 0;

			switch (n) {
				case 1:
					Trick_Trick(74, "180 OLLIE", -50, 0);
					break;
				case 2:
					Trick_Trick(74, "360 OLLIE", -100, 0);
					break;
				case 3:
					Trick_Trick(74, "540 OLLIE", -150, 0);
					break;
				case 4:
					Trick_Trick(74, "720 OLLIE", -200, 0);
					break;
				case 5:
					Trick_Trick(74, "900 OLLIE", -250, 0);
					break;
				case 6:
					Trick_Trick(74, "1080 OLLIE", -300, 0);
					break;
				case 7:
					Trick_Trick(74, "MEGA OLLIE", -350, 0);
					break;
			}

			m364 = 1;
		}
	}

	Panel_Land(this);

	if (m364) {
		if (mpWallItem && Mem_CheckHandle(&mWallHandle)) {
			TrickOb_TrickedOnItem(mpWallItem, this, mPanel.mLandedTotal);

			mpWallItem = 0;
		}

		if (m364 && GGame == 7) {
			printf("Setting Playtime to 0\n");

			mTime = 0;
		}
	}

	Crowd_Cheer(mPanel.mLandedTotal);
	unsigned Cheer = mPanel.mLandedTotal;

	if (m364) {
		if (!mBalanceFlash) {
			mBalanceTime += Cheer;

			if (mBalanceTime >= 3000) {
				mBalanceTime = 0;
				mBalanceFlash = 750;
				SetGlow(20, 20, 20, 5);
			}
		} else {
			mBalanceFlash += Cheer / 5;

			if (mBalanceFlash > 750)
				mBalanceFlash = 750;
		}
	}

	m364 = 0;
	m368 = 0;
	mNum180Spins = 0;
	mTallyAngles = 0;
	m760 = 0;
}

/* Trick_Bail__6CBruce (0x8002f774) */
/* A bail ends the combo: clears trick state, cheers or groans the crowd by the
   trick queue's score (or the bail speed), forgets this combo's trick counts, and rumbles. */
void CBruce::Trick_Bail() {
	int i, Score, Speed, Spins;

	if (GGame == 7)
		mTime = 0;

	m760 = 0;
	mNum180Spins = 0;
	mTallyAngles = 0;
	mGaps = 0;
	mpGrindGapTrick = 0;
	m720 = 0;
	mNoSpinScore = 0;
	mNoSpin = 0;
	mBalanceTime = 0;
	mBalanceFlash = 0;
	SetGlow(0, 0, 0, 10);

	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;
	Score = 0;

	for (i = 0; i < mPanel.mTQStack; i++)
		Score += mPanel.mTQ[i].point_value;

	Spins = mNum180Spins;

	if (Spins < 3)
		Score = Score * (mPanel.mTQStack * 2 + Spins) / 2;
	else
		Score = (Score * (mPanel.mTQStack * 2 + (Spins * 2 - 2))) >> 1;

	if (Score != 0)
		Crowd_Ahh(Score);
	else
		Crowd_Oww(Speed >> 12);

	for (i = 0; i < 80; i++) {
		m428[i] -= m568[i];
		m568[i] = 0;
	}

	Panel_Bail(this);

	m364 = 0;
	m368 = 0;
	Vibrate(8, 1, 150);
}

/* Vibrate__6CBruceiii (0x8002f8f0) */
/* Runs the pad's actuator, unless vibration is off for this player (horse swaps
   pads) or a replay video is playing. */
void CBruce::Vibrate(int a, int b, int c) {
	int Pad = mPadIndex;

	if (GGame == 7)
		Pad ^= GHorsePlayer;

	if (GCareer.Options.VibrationEnabled[Pad] == 0)
		return;

	if (GVInfo1[0].VideoMode == 2)
		return;

	if (GVInfo1[1].VideoMode == 2)
		return;

	Pad_ActuatorOn(Pad, a, b, c);
}

/* StateTransition__6CBruce13EPhysicsStatei (0x8002f980) */
/* Changes physics state: clears rail state on leaving a rail, counts airs,
   remembers the wall when going onto an invisible wall, and leaving a wallride scores it and
   turns the skater's right/up axes back off the wall. */
void CBruce::StateTransition(EPhysicsState NewState, int) {
	CVector Old;

	if (mPhysicsState == PHYSICS_ON_RAIL && NewState == PHYSICS_IN_AIR) {
		m264 = 0;
		m260 = 0;
		m280 = 0;
		m27C = 0;
		m270 = 0;
		m274 = 0;
		m26C = 0;
	}

	if (mPhysicsState != NewState) {
		if (NewState == PHYSICS_IN_AIR && mPhysicsState != PHYSICS_ON_INVISIBLE)
			mAirs++;

		if (NewState == PHYSICS_ON_INVISIBLE && mPhysicsState != PHYSICS_IN_AIR)
			mAirs++;
	}

	if (mPhysicsState != PHYSICS_ON_INVISIBLE && NewState == PHYSICS_ON_INVISIBLE) {
		mFallLine = mFront;
		mFallLine.vx = -Mips_FixMul(mFallLine.vx, 13000);
		mFallLine.vy = Mips_FixMul(mFallLine.vy, 13000);
		mFallLine.vz = -Mips_FixMul(mFallLine.vz, 13000);
	}

	if (mPhysicsState == PHYSICS_IN_WALLRIDE && NewState == PHYSICS_IN_AIR) {
		Trick_Trick(64, "WALL RIDE", 500, 0);

		int Flipped = (mExtraFlags & 2) != 0;

		if (mAnim == 35 ? !Flipped : Flipped) {
			Old = mRight;
			mRight = mUp;
			mUp = -Old;
		} else {
			Old = mRight;
			mRight = -mUp;
			mUp = Old;
		}
	}

	RotMatrixFromFRUVectors();

	mPhysicsState = NewState;
}

static inline void FlipAxis(short &Matrix, int &Axis) {
	Matrix = -Axis;
	Axis = Matrix;
}

/* MaybeFlipAnimationTransition__6CBruce (0x8002fc0c) */
/* Applies a pending flip before an animation change: toggles the flipped flag,
   and/or mirrors the front and right axes (negating the matching rotation matrix columns). */
void CBruce::MaybeFlipAnimationTransition() {
	if (mFlipPending) {
		mFlipPending = 0;
		mExtraFlags ^= 2;
	}

	if (mFlipAxesPending) {
		mFlipAxesPending = 0;
		FlipAxis(mRotMatrix.m[0][2], mFront.vx);
		FlipAxis(mRotMatrix.m[1][2], mFront.vy);
		FlipAxis(mRotMatrix.m[2][2], mFront.vz);
		FlipAxis(mRotMatrix.m[0][0], mRight.vx);
		FlipAxis(mRotMatrix.m[1][0], mRight.vy);
		FlipAxis(mRotMatrix.m[2][0], mRight.vz);
	}
}

/* Debug_RunAnim__6CBruceii (0x8002fcdc) */
/* RunAnim(Anim) after any pending flip; Line is the caller's __LINE__. */
void CBruce::Debug_RunAnim(int Anim, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, 0, -1, -1);
}

/* Debug_RunAnim1__6CBruceiii (0x8002fd28) */
/* RunAnim(Anim, a) after any pending flip; Line is the caller's __LINE__. */
void CBruce::Debug_RunAnim1(int Anim, int a, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, -1, -1);
}

/* Debug_RunAnim2__6CBruceiiii (0x8002fd80) */
/* RunAnim(Anim, a, b) after any pending flip; Line is the caller's __LINE__. */
void CBruce::Debug_RunAnim2(int Anim, int a, int b, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, b, -1);
}

/* Debug_RunAnim3__6CBruceiiiii (0x8002fde4) */
/* RunAnim(Anim, a, b, c) after any pending flip; Line is the caller's __LINE__. */
void CBruce::Debug_RunAnim3(int Anim, int a, int b, int c, int) {
	MaybeFlipAnimationTransition();
	RunAnim(Anim, a, b, c);
}

/* ResetTrickBuffers__6CBruce (0x8002fe50) */
/* Clears the buffered trick inputs; the older buffer only if it is more than 8
   frames old. */
void CBruce::ResetTrickBuffers() {
	int Age = XFrames - m274;

	mTapTurn = 0;
	m370 = 0;
	m36C = 0;
	m3A0[0] = 0;
	m378[0] = 0;

	if (Age > 8) {
		m280 = 0;
		m27C = 0;
		m274 = 0;
	}

	m264 = 0;
	m260 = 0;
	m270 = 0;
	m26C = 0;
}

/* Bail__6CBrucei (0x8002fe9c) */
/* Starts a bail: ends the combo, goes into the air without the board, runs the
   bail animation (harder above a speed), and plays a skater-specific (or random) bail sound. */
void CBruce::Bail(int BailNumber) {
	mBailNumber = BailNumber;
	m724 = 0;
	mBails++;
	Trick_Bail();

	m71C = 0;
	mInAir = 1;
	mJumpPending = 0;
	mJumpStrength = 0;
	ResetTrickBuffers();

	mDoingATrick = 0;
	mStickToRail = 0;
	StateTransition(PHYSICS_IN_AIR, 0x412);
	BoardOff();

	mAnimSpeed = 0x10000;
	Debug_RunAnim2(0x36, 0, 0x1d, 0x416);

	if (0x2d000 < (M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6))
		m24C = 1;
	else
		m24C = 6;

	FlipToVelocity();

	if (Stats[mSkaterIndex].ComboID == 0x9000 || Stats[mSkaterIndex].ComboID == 0xb000) {
		SFX_PlayX(SFX_BAIL_FEMALE, 0x2000, 0);
		SFX_PlayX(SFX_SKID, 0x2000, 0);
	} else {
		if (Rnd(2))
			SFX_PlayX(SFX_BAIL_MALE_1, 0x2000, 0);
		else
			SFX_PlayX(SFX_BAIL_MALE_2, 0x2000, 0);
	}

	m6DC = 0;
}

/* ProjectCVectorToPlane__FP7CVectorG7SVECTOR (0x8002fff4) */
void ProjectCVectorToPlane(CVector *v, SVECTOR n) {
	CVector t;
	int d;

	t.vx = n.vx;
	t.vy = n.vy;
	t.vz = n.vz;
	d = Mips_CVecDotProd(v, &t);
	v->vx -= Mips_FixMul(d, n.vx);
	v->vy -= Mips_FixMul(d, n.vy);
	v->vz -= Mips_FixMul(d, n.vz);
}

/* CVectorRotateToPlane__FP7CVectorP7SVECTORT0 (0x80030090) */
void CVectorRotateToPlane(CVector *InVector, SVECTOR *Normal, CVector *OutVector) {
	int OldMag, NewMag;
	CVector t;

	OldMag = M3dMaths_SquareRoot0(Mips_CVecDotProd(InVector, InVector)) << 6;
	*OutVector = *InVector;
	ProjectCVectorToPlane(OutVector, *Normal);

	NewMag = M3dMaths_SquareRoot0(Mips_CVecDotProd(OutVector, OutVector)) << 6;
	OldMag >>= 8;
	NewMag >>= 8;
	__asm__("" : "=m"(t));

	*OutVector = (*OutVector * OldMag) / NewMag;
}

/* UberFrig__FP6CBruceG7CVectorPc (0x800301b0) */
void UberFrig(CBruce *p, CVector a, char *msg) {
	SLineInfo li;

	if (p->m6DC == 0) {
		li.StartCoords.vx = a.vx;
		li.StartCoords.vy = a.vy;
		li.StartCoords.vz = a.vz;
		li.EndCoords = li.StartCoords;
		li.EndCoords.vy = li.EndCoords.vy + 0x1f40000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.pItem) {
			p->CalculateDynamicLighting(li.pFace, li.pItem);
			return;
		}

		li.StartCoords.vx = a.vx;
		li.StartCoords.vy = a.vy;
		li.StartCoords.vz = a.vz;
		li.StartCoords.vy = a.vy + -0x12c000;
		li.EndCoords = li.StartCoords;
		li.EndCoords.vy = li.EndCoords.vy + 0x190000;
		M3dColij_InitLineInfo(&li);
		M3dZone_LineToItemNotSCached(&li, 1);

		if (li.pItem) {
			p->mPos.vx = li.Position.vx;
			p->mPos.vy = li.Position.vy;
			p->mPos.vz = li.Position.vz;
			p->mPos.vy = li.Position.vy + -0x1e000;
			p->mOldPos = p->mPos;
			return;
		}

		p->mPos = p->mSafePos;
		p->mOldPos = p->mPos;
		p->mVel.vx = -p->mVel.vz / 2;
		p->mVel.vz = p->mVel.vx / 2;
	}
}

/* FrigDarkenObject__6CBruce (0x8003036c) */
/* Awards the combo to the wall item (when its handle is still live): the sum
   of the queued trick scores (mPanel.mTQ) times a multiplier from the trick count and the
   180-spin count, halved, passed to TrickOb_TrickedOnItem. */
void CBruce::FrigDarkenObject() {
	int i, sum, n, score;

	if (!mpWallItem || !Mem_CheckHandle(&mWallHandle))
		return;

	sum = 0;
	n = mPanel.mTQStack;

	for (i = 0; i < n; i++)
		sum += mPanel.mTQ[i].point_value;

	{
		int b = mNum180Spins;
		int n2 = n * 2;

		if (b < 3)
			score = sum * (n2 + b) / 2;
		else {
			int t = n2 - 2;

			score = sum * (t + b * 2) / 2;
		}
	}

	if (score > 0)
		TrickOb_TrickedOnItem(mpWallItem, this, score);
}

/* TrickObRail__6CBruce (0x80030434) */
/* THPS1 fills in the demo's empty TrickObRail: when the rail start node (type 11) links to a trick
   object node (type 14 or 12), look up its enviro item by checksum and remember it with a heap ref.
   Lever: the second link test uses its own pointer local (reusing pLink changes the branch layout). */
void CBruce::TrickObRail() {
	short *pNode;

	if (*OffsetList[mRailStart] == 11) {
		ushort *pLinks = Trig_GetLinksPointer(mRailStart);
		short *pLink = OffsetList[pLinks[1]];

		if (*pLink == 14 || *pLink == 12)
			pNode = pLink;
		else {
			short *pLink2 = OffsetList[pLinks[2]];

			if (*pLink2 == 14 || *pLink2 == 12)
				pNode = pLink2;
		}

		pNode += (ushort)pNode[1] + 2;

		if ((intptr_t)pNode & 3)
			pNode++;

		uint Checksum = *(uint *)pNode;

		FrigDarkenObject();

		mpWallItem = Spool_FindEnviroItem(Checksum);
		mWallHandle = Mem_GetHandle(PSXRegion[mpWallItem->Region].pPSX);

		if (GGame == 1)
			TrickOb_TrickedOnItem(mpWallItem, this, -1);
	}
}

inline void CBruce::StartHandPlant(int RailTrick, int Flags) {
	mAnimSpeed = 0x10000;
	m248 = 0;

	if (!m760) {
		Trick_TakeOff();

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
		SFX_PlayX(SFX_SPECIAL_TRICK, 0x2000, 0);
	} else {
		Trick_Trick(79, "Handplant", 1000, 0);
		Debug_RunAnim1(34, 8, 1544);

		m368 |= 0x4800;
	}

	StateTransition(PHYSICS_IN_HANDPLANT, 1548);
	SFX_PlayX(SFX_IMPACT_1, 0x2000, 0);

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
			SFX_PlayX((ESfx)(SFX_RAIL_LAND_1 + Rnd(2)), 0x2000, 0);
			break;
		case 2:
			SFX_PlayX((ESfx)(SFX_IMPACT_1 + Rnd(2)), 0x2000, 0);
			break;
	}

	StateTransition(PHYSICS_ON_RAIL, 1667);

	mDoingATrick = 1;
	m424++;
	mRails++;
	TrickObRail();
	Mips_CVecDotProd(&mRailDir, &mFront);

	RightDot = Mips_CVecDotProd(&mRailDir, &mRight);
	m320 = 0;

	if (RailTrick) {
		SFX_PlayX(SFX_SPECIAL_TRICK, 0x2000, 0);

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

/* HandleStickToRail__6CBruce (0x8003058c) */
/* Called from DoPhysics where the demo calls HandleStickToRail. THPS1 keeps the
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
   twice, so no sched1 set-once boost; as in CBruce::ScoreCurrentTrick) and read after the mGrindTrick store.
   Residues: the trick walk and the GetBalanceVel copy/arm register roles (a1/a2/a3: retail's DAT
   arm uses the load, the stat arms the copy; ours the reverse = cse1 swapped `321=mem; 322=321`
   into `322=mem; 321=322` because 322 outlives the block).
   Trick walk: retail's zero-trip `blez` goes straight to `RailTrick = 0` (jump-threaded past the
   post-loop recheck). The test written WITHOUT `f` (`Group[RailTrick].Flags > 0`) threads
   it, after which gcse PRE copies 2*RailTrick in both body and test and loop reduces the 2R giv
   exactly as retail (li 64 / +2), but still 2 words long: retail's entry load and loop test share
   one pseudo (t0; unboosted, so the entry lw lands early), and ours are two (renamed by
   duplicate_loop_exit_test), which also moves the 2R biv out of v0. With `f`, no threading. */
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
		Bail(0);

		mTrickBails++;
		return;
	}

	if (mDoingATrick) {
		ScoreCurrentTrick();

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

/* HandleBailOnLanding__6CBrucei (0x80031108) */
/* On landing: bails if a held trick button is still down, if not upright, or
   if a rail landing is too far off line; counts it as a trick bail. */
void CBruce::HandleBailOnLanding(int) {
	if (mInAir)
		return;

	if (!mDoingATrick && !m720)
		return;

	int Bailing = 0;

	if (m368 & 0x2000)
		Bailing = mpPad->Flip.Pressed != 0;

	if (m368 & 0x4000)
		Bailing |= mpPad->Grab.Pressed != 0;

	Bailing |= m360 == 0;

	if (m720) {
		if (m728 > 0) {
			if ((m724 & 4095) < 3796)
				Bailing = 1;
		} else {
			if ((m724 & 4095) >= 301)
				Bailing = 1;
		}
	}

	if (Bailing) {
		/* The retail call leaves the limit argument unset; Bail ignores it. */
		Bail(0);

		mTrickBails += 1;
	}
}

static inline void BailSetVector(VECTOR *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

/* HandleBailPerpendicular__6CBruceG7SVECTOR (0x80031224) */
/* Fuzzy demo twin (0.83; retail is trimmed), locals from its MAIN.SYM. When the
   skater runs into a wall (Normal) fast and nearly head-on, flips them, faces them back along
   their last movement, rebuilds the orientation against the wall normal and, if they were
   facing away, starts the perpendicular bail. */
void CBruce::HandleBailPerpendicular(SVECTOR Normal) {
	CVector InplaneVel;
	int Speed;
	int Dot;

	InplaneVel = mVel;
	ProjectCVectorToPlane(&InplaneVel, Normal);

	Speed = SquareRoot0(Mips_CVecDotProd(&InplaneVel, &InplaneVel)) << 6;

	if (Speed <= 0x1e000)
		return;

	Dot = (Mips_CVecDotProd(&InplaneVel, &mFront) << 6) / (Speed >> 6);

	if (ABS(Dot) >= 1638)
		return;

	FlipToVelocity();

	{
		int RightDot = Mips_CVecDotProd(&InplaneVel, &mRight);

		Bail(0);

		mPerpendicularBails++;

		{
			VECTOR v;
			CVector vv;

			vv = mOldPos - m1B0;
			BailSetVector(&v, vv.vx, vv.vy, vv.vz);

			if (ABS(v.vx) + ABS(v.vy) + ABS(v.vz) > 0x8000)
				BailSetVector(&v, v.vx >> 4, v.vy >> 4, v.vz >> 4);

			VectorNormal(&v, &v);

			mFront.vx = -v.vx;
			mFront.vy = -v.vy;
			mFront.vz = -v.vz;
			mUp.vx = Normal.vx;
			mUp.vy = Normal.vy;
			mUp.vz = Normal.vz;
			geom_cross(&mFront, &mUp, &mRight, 12);
			geom_cross(&mUp, &mRight, &mFront, 12);
			RotMatrixFromFRUVectors();
		}

		if (mExtraFlags & 2)
			RightDot = -RightDot;

		if (RightDot <= 0) {
			mAnimSpeed = 0x10000;
			Debug_RunAnim(46, 1997);

			m24C = 5;
		}
	}
}

/* HandleHoldTrick__6CBruce (0x80031500) */
/* While a held trick is being scored (m368 & 0x800, trick in the low byte,
   0x4000/0x2000 states), adds 1/20 of its depreciated table score via Trick_HoldTrick. */
void CBruce::HandleHoldTrick() {
	int f = m368;
	int n = 0;

	if (f & 0x800) {
		int t = f & 0xff;

		if (f & 0x6000)
			n = Trick_RepeatScore(t, Group[t].Score) / 20;

		if (n)
			Trick_HoldTrick(n);
	}
}

/* ScoreCurrentTrick__6CBruce (0x8003159c) */
/* Called by HandleStickToRail when a trick is in progress (m35C): unless already
   scored (m364), plays the landing sound for anims 62..66 and scores the held trick (m368 & 0xff,
   0x4000/0x2000 states, the same test as the Limit block there) via Trick_Trick from the 44-byte
   trick table, counting it in m424, then sets 0x800 (the 0x4000 state always, the 0x2000 state only when the trick
   has frames; retail's `j L66` is the cross-jumped `m368 |= 0x800` tail).
   Score is declared at function scope and assigned in both arms: a pseudo set twice loses
   sched1's set-once priority boost, so its load stays above the m424 increment as retail has it.) */
void CBruce::ScoreCurrentTrick() {
	if (m364)
		return;

	if ((ushort)(mAnim - 62) < 5)
		SFX_PlayX(SFX_SPECIAL_TRICK, 0x2000, 0);

	int State = m368;
	int Trick = State & 0xff;
	int Score;

	if (State & 0x4000) {
		Score = Group[Trick].Score;
		m424++;
		Trick_Trick(Trick, Group[Trick].pName, Score, 0);

		m368 |= 0x800;
	} else if (State & 0x2000) {
		Score = Group[Trick].Score;
		m424++;
		Trick_Trick(Trick, Group[Trick].pName, Score, 0);

		if (Group[Trick].Frames)
			m368 |= 0x800;
	}
}

/* SetTrickFlags__6CBruceP6STrick (0x800316cc) */
/* PHYSICS: copies a trick-table entry's flag bits (top 12 bits of its first word) into the
   skater; called twice from HandleTricksNew__6CBruce with &Group[i]. */
void CBruce::SetTrickFlags(STrick *pTrick) {
	unsigned Flags = pTrick->Flags & 0xfff00000;

	mNoSpin = (Flags & 0x100000) != 0;
	mNoSpinScore = (Flags & 0x200000) != 0;
	m1EC = (Flags & 0x4000000) != 0;

	if (mNoSpin)
		mTapTurn = 0;

	if (Flags & 0x800000) {
		m368 = (m368 & 0xffffff) | 0x80000000;
		m368 &= ~0x4000;
		m368 |= 0x2000;
	}

	mFlipPending = (Flags & 0x1000000) != 0;
	mFlipAxesPending = (Flags & 0x400000) != 0;
}

/* HandleTricksNew__6CBruce (0x80031758) */
/* Called from DoPhysics. THPS1's whole trick-input handler; the THPS2 demo's
   HandleTricksNew is a 76-byte wrapper around its new button-queue code, so there is no usable
   twin. Records the d-pad direction and the Circle/Square buttons into 10-slot ring buffers
   (m378 dirs, m3A0 buttons, m3C8/m3F0 their times), with m25C..m280 remembering the previous
   direction for special (two-direction) combos. A buffered entry starts a trick: specials are
   searched from trick 32 by style/combo/dir/button key (needs special meter mBalanceFlash), otherwise
   Circle = flip T[dir+16], Square = grab T[dir]. Then the held-trick state machine: the 0x40000000
   (held anim) / 0x20000000 / 0x80000000 states release into anim 14. Line numbers are the
   Debug_RunAnim __LINE__s.
   Levers: the paired ring-slot clears are chained (`m378[k] = m3A0[k] = 0;`) so the index is
   loaded once; the special-trick search walks a pointer (`p = &T[32], i = 32; ...; i++, p++`);
   mAnimSpeed is read before m368 is set; the combo-timing test computes Gap in a nested block. */
void CBruce::HandleTricksNew() {
	if (!mDoingATrick && !m740)
		m424 = 0;

	if (m200) {
		m200--;
		return;
	}

	if (m724)
		return;

	int Dir = mpPad->Up.Pressed + mpPad->Down.Pressed * 2 + mpPad->Left.Pressed * 4 +
	          mpPad->Right.Pressed * 8;

	if (mAY < -40)
		Dir |= 1;

	if (mAY > 40)
		Dir |= 2;

	if (mAX < -40)
		Dir |= 4;

	if (mAX > 40)
		Dir |= 8;

	if (Dir) {
		if (m280) {
			if (m27C) {
				m25C = m268;
				m260 = m26C;
				m264 = m270;
				m268 = m278;
				m26C = m274;
				m270 = m27C;
				m274 = 0;
				m27C = 0;
			}

			if (!m274) {
				m278 = Dir;
				m27C = 0;
				m274 = XFrames;
			}
		}

		m378[m370] = Dir;
		m3C8[m370] = XFrames;

		if (m3A0[m370] && XFrames - m3F0[m370] > 20)
			m3A0[m370] = 0;
	} else if (m274 && !m27C) {
		m27C = XFrames;
	} else {
		m280 = 1;
	}

	if (mInAir || !m740 || mPhysicsState == 4 || mPhysicsState == 5 || mPhysicsState == 8) {
		m374 = 0;
		return;
	}

	int Button = mpPad->Grab.Pressed + mpPad->Flip.Pressed * 2;

	if (Button) {
		m3A0[m370] = Button;
		m3F0[m370] = XFrames;

		if (m378[m370] && XFrames - m3C8[m370] > 20)
			m378[m370] = 0;
	}

	if (!Button) {
		if (m374 > 0) {
			m3A0[m370] = m378[m370] = 0;
			m374--;
		}
	}

	if (m378[m370] && m3A0[m370] && !m374) {
		if (m270 && m270 < m3C8[m370]) {
			int Gap = m3C8[m370] - m270;

			if (m270 - m26C < 20 && Gap < 30)
				m378[m370] |= m268 << 4;
		}

		m264 = 0;
		m260 = 0;
		m280 = 0;
		m27C = 0;
		m270 = 0;
		m274 = 0;
		m26C = 0;

		if (++m370 == 10)
			m370 = 0;

		m378[m370] = m3A0[m370] = 0;
		m374 = 3;
	}

	if (m36C != m370 && (!mDoingATrick || m360)) {
		int Entry = m378[m36C];

		Button = m3A0[m36C];
		Dir = Entry & 15;
		int Combo = Entry >> 4;

		if (!mBalanceFlash)
			Combo = 0;

		if (Combo) {
			int Key = Stats[mSkaterIndex].ComboID + (Combo << 8) + (Dir << 4) + Button;
			int i;
			STrick *p;

			for (p = &Group[32], i = 32; p->Flags > 0; i++, p++) {
				if ((p->Flags & 0xfffff) == Key) {
					mAnimSpeed = p->w2;

					if (Button == 2)
						m368 = i | 0x2000;
					else
						m368 = i | 0x4000;

					if (p->Frames) {
						Debug_RunAnim2(p->Anim, 0, p->Frames, 2335);

						m368 |= 0x40000000;
					} else {
						Debug_RunAnim(p->Anim, 2341);

						if (Button == 2)
							m368 |= 0x80000000;
						else
							m368 |= 0x20000000;
					}

					SetTrickFlags(p);

					m720 = p->w8;
					m728 = p->w9;

					if (m728 < 0)
						m720++;

					m724 = 0;

					if (mExtraFlags & 2)
						mTapTurn -= p->w10;
					else
						mTapTurn += p->w10;

					mDoingATrick = 1;
					m364 = 0;
					m360 = 0;
					m348 = XFrames;
					m378[m36C] = m3A0[m36C] = 0;

					if (++m36C == 10)
						m36C = 0;

					break;
				}
			}

			if (p->Flags == -1)
				Combo = 0;
		}

		if (!Combo && (Button & 1) && Group[Dir].Anim != 78) {
			Dir += 16;
			mAnimSpeed = Group[Dir].w2;
			m368 = Dir | 0x4000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2(Group[Dir].Anim, 0, Group[Dir].Frames, 2396);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim(Group[Dir].Anim, 2401);

				m368 |= 0x20000000;
			}

			mDoingATrick = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		} else if (!Combo && (Button & 2) && Group[Dir].Anim != 78) {
			mAnimSpeed = Group[Dir].w2;
			m368 = Dir | 0x2000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2(Group[Dir].Anim, 0, Group[Dir].Frames, 2423);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim(Group[Dir].Anim, 2428);

				m368 |= 0x80000000;
			}

			mDoingATrick = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		}
	}

	if (!mDoingATrick)
		return;

	int State = m368 & 0xff000000;
	int Trick = m368 & 0xff;

	if (State == 0x20000000 && mAnimFinished)
		m360 = 1;

	if (State == 0x40000000 && mAnimFinished)
		m360 = 1;

	if (mAnimDir == -1)
		m360 = 1;
	else {
		if ((m368 & 0x4000) && Group[Trick].Frames && mFrame >= mNumFrames - Group[Trick].w5 - 1)
			m360 = 1;

		if ((m368 & 0x2000) && mFrame >= mNumFrames - Group[Trick].w5 - 1)
			m360 = 1;
	}

	if (m360) {
		ScoreCurrentTrick();

		m364 = 1;
	}

	if (!m1EC)
		HandleHoldTrick();

	if (!mAnimFinished)
		return;

	State = m368 & 0xff000000;
	Trick = m368 & 0xff;

	switch (State) {
		case 0x40000000:
			if (m368 & 0x4000) {
				if (mpPad->Grab.Pressed)
					m368 |= 0x800;

				if (!mpPad->Grab.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2(mAnim, Group[Trick].Frames, -1, 2529);
					else if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2535);

						mDoingATrick = 0;
					}
				}
			} else {
				if (mpPad->Flip.Pressed)
					m368 |= 0x800;

				if (!mpPad->Flip.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2(mAnim, Group[Trick].Frames, -1, 2553);
					else if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2559);

						mDoingATrick = 0;
					}
				}
			}

			break;
		case 0x20000000:
			if (mpPad->Grab.Pressed)
				m368 |= 0x800;

			if (!mpPad->Grab.Pressed)
				m368 &= ~0x800;

			if (!(m368 & 0x800)) {
				if (m368 & 0x400) {
					if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2580);

						mDoingATrick = 0;
					}
				} else {
					Debug_RunAnim2(mAnim, mFrame, 0, 2586);

					m368 |= 0x400;
				}
			}

			break;
		case (int)0x80000000:
			if (!m6DC) {
				mAnimSpeed = 0x10000;
				Debug_RunAnim(14, 2596);

				mDoingATrick = 0;
			}

			break;
	}
}

/* InterpolateFrame__Fii (0x8003237c) */
int InterpolateFrame(int cur, int tgt) {
	int d, s, m;

	if (cur == tgt)
		return cur;

	d = tgt - cur;
	s = 1;

	if (d < 0)
		s = -1;

	m = d >> 31;
	d = (d ^ m) - m;

	if (d > 12)
		return cur + s * 5;

	if (d >= 4)
		return cur + s * 3;

	return cur + s;
}

/* HandleSpin__6CBruce (0x800323e0) */
/* Demo twin (fuzzy), locals from its MAIN.SYM. Called from DoPhysics right
   after FRUVectorsFromRotMatrix. Spinning and leaning: THPS1 replaces the demo's rotate-speed
   loop with L2/R2 tap turns (mTapTurn counts queued 180s; each frame rotates the heading and
   tallies the angle, scoring a 180 every 2048 units). Otherwise it picks the turn rate and the
   maximum lean from the board's turning stat (on the ground) or the shoulder buttons (in the
   air), leans toward the analogue target or the d-pad direction, decays the lean when nothing
   is pressed, then (inline here; a separate CalcAnimLeanFrame in the demo) picks the lean anim
   frame, and finally sets the pitch from up/down.
   Levers: every MaxLeanAngle assignment is a positive constant (THPS1 writes 0x5a000 where the
   demo doubled it), so combine's nonzero_bits proves it non-negative and (Max >> 12) / 2 becomes
   retail's single `srl 13`; the d-pad anim-lean clamp only stores when in range
   (if (a - d >= -Max) a -= d;), giving the two arms' shared `slt; bne; sw` tail; the anim-range
   tests are switches (signed slti trees); DeltaAngle's per-stat rate is one ?: expression. */
void CBruce::HandleSpin() {
	int DeltaAngle;
	int MaxLeanAngle;
	int LeftPressed;
	int RightPressed;

	if (mPhysicsState == 1 || mPhysicsState == 2) {
		if (!mNoSpin) {
			if (mpPad->TapSpinLeft.Triggered) {
				mpPad->TapSpinLeft.Triggered = 0;

				if (mTapTurn > 0)
					mTapTurn = 0;

				mTapTurn--;
			}

			if (mpPad->TapSpinRight.Triggered) {
				mpPad->TapSpinRight.Triggered = 0;

				if (mTapTurn < 0)
					mTapTurn = 0;

				mTapTurn++;
			}
		}

		if (mTapTurn > 0) {
			RotateHeadingAndVelocity((TimeScale * 150) >> 8, 0, 0);

			mTallyAngles += (TimeScale * 150) >> 8;

			if (mTallyAngles >= 2048) {
				mTallyAngles -= 2048;
				mTapTurn--;

				if (!mNoSpinScore)
					mNum180Spins++;
			}
		}

		if (mTapTurn < 0) {
			RotateHeadingAndVelocity(-((TimeScale * 150) >> 8), 0, 0);

			mTallyAngles -= (TimeScale * 150) >> 8;

			if (mTallyAngles <= -2048) {
				mTallyAngles += 2048;
				mTapTurn++;

				if (!mNoSpinScore)
					mNum180Spins++;
			}
		}
	} else
		mTapTurn = 0;

	if (mTapTurn) {
		mLeanAngle = 0;
		return;
	}

	mTurning = 0;

	if (mPhysicsState == 0) {
		DeltaAngle = (TimeScale * (mpStatBonus[4] == 1   ? 10240
		                           : mpStatBonus[4] == 3 ? 15872
		                                                 : 30720)) >>
		             8;
		MaxLeanAngle = 0x2d000;
		LeftPressed = mpPad->Left.Pressed;
		RightPressed = mpPad->Right.Pressed;
		mFirstLeftRightPressed = 0;

		if (mAY >= 31 || mpPad->Down.Pressed) {
			mDoingSharpTurn = 1;

			{
				int Speed = (M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6) >> 12;

				DeltaAngle *= 2;
				MaxLeanAngle = 0x5a000;
				mAcc.vx -= mVel.vx / Speed;
				mAcc.vy -= mVel.vy / Speed;
				mAcc.vz -= mVel.vz / Speed;
			}
		}
	} else {
		if (Pad[0].SpinLeft.Pressed || Pad[0].SpinRight.Pressed)
			DeltaAngle = (TimeScale * 0x64000) >> 8;
		else
			DeltaAngle = (TimeScale * 0xa000) >> 8;

		MaxLeanAngle = 0xa0000;
		LeftPressed = mpPad->Left.Pressed || mpPad->SpinLeft.Pressed;
		RightPressed = mpPad->Right.Pressed || mpPad->SpinRight.Pressed;

		if (LeftPressed || RightPressed) {
			if (mFirstLeftRightPressed > 0 && XFrames - mFirstLeftRightPressed < 8) {
				LeftPressed = 0;
				RightPressed = 0;
			}
		} else
			mFirstLeftRightPressed = XFrames;

		LeftPressed |= mpPad->SpinLeft.Pressed;
		RightPressed |= mpPad->SpinRight.Pressed;

		if (mNoSpin)
			LeftPressed = RightPressed = 0;

		if (!LeftPressed && !RightPressed && !mAX)
			mLeanAngle = 0;

		mDoingSharpTurn = 0;
	}

	if (mAX && !LeftPressed && !RightPressed) {
		int TargetAngle = mAX * MaxLeanAngle / 128;

		if (TargetAngle < -MaxLeanAngle)
			TargetAngle = -MaxLeanAngle;

		if (TargetAngle > MaxLeanAngle)
			TargetAngle = MaxLeanAngle;

		if (mLeanAngle < TargetAngle) {
			int Diff = (TargetAngle - mLeanAngle) >> 12;

			mTurning = 1;
			mLeanAngle += DeltaAngle + DeltaAngle * Diff / ((MaxLeanAngle >> 12) / 2);

			if (mLeanAngle > TargetAngle)
				mLeanAngle = TargetAngle;

			mAnimLeanAngle += DeltaAngle;

			if (mAnimLeanAngle > TargetAngle)
				mAnimLeanAngle = TargetAngle;
		}

		if (mLeanAngle > TargetAngle) {
			int Diff = (mLeanAngle - TargetAngle) >> 12;

			mTurning = 1;
			mLeanAngle -= DeltaAngle + DeltaAngle * Diff / ((MaxLeanAngle >> 12) / 2);

			if (mLeanAngle < TargetAngle)
				mLeanAngle = TargetAngle;

			mAnimLeanAngle -= DeltaAngle;

			if (mAnimLeanAngle < TargetAngle)
				mAnimLeanAngle = TargetAngle;
		}
	} else if (LeftPressed) {
		mTurning = 1;
		mLeanAngle -= DeltaAngle;

		if (mLeanAngle < -MaxLeanAngle)
			mLeanAngle = -MaxLeanAngle;

		if (mAnimLeanAngle - DeltaAngle >= -MaxLeanAngle)
			mAnimLeanAngle -= DeltaAngle;
	} else if (RightPressed) {
		mTurning = 1;
		mLeanAngle += DeltaAngle;

		if (mLeanAngle > MaxLeanAngle)
			mLeanAngle = MaxLeanAngle;

		if (mAnimLeanAngle + DeltaAngle <= MaxLeanAngle)
			mAnimLeanAngle += DeltaAngle;
	} else {
		int Step;

		if (mDoingSharpTurn)
			mLeanAngle -= mLeanAngle >> 1;
		else
			mLeanAngle -= mLeanAngle >> 2;

		if (((mLeanAngle + 2048) >> 12) == 0)
			mLeanAngle = 0;

		Step = mAnimLeanAngle >> 4;

		if (Step < 0)
			if (Step > -1024)
				Step = -1024;

		if (Step > 0 && Step < 1024)
			Step = 1024;

		mAnimLeanAngle -= Step;

		if (((mAnimLeanAngle + 2048) >> 12) == 0)
			mAnimLeanAngle = 0;
	}

	mAnimLeanAngle = mLeanAngle;

	if (mAnimLeanAngle) {
		if (!mManIsCrouched) {
			switch (mAnim) {
				case 0:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (mExtraFlags & 2) {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 22) / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 7)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(7, mFrame, mFrame, 2881);
						} else {
							int Frame = mAnimLeanAngle * 22 / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 6)
								mFrame = 0;

							if (mFrame < Frame)
								mFrame++;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(6, mFrame, mFrame, 2892);
						}
					} else {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 22) / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 6)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(6, mFrame, mFrame, 2906);
						} else {
							int Frame = mAnimLeanAngle * 22 / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 7)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(7, mFrame, mFrame, 2916);
						}
					}
			}
		}
	}

	if (mAnimLeanAngle) {
		if (mManIsCrouched) {
			switch (mAnim) {
				case 0:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (mExtraFlags & 2) {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 12) / MaxLeanAngle;

							if (Frame > 12)
								Frame = 12;

							mAnimSpeed = 0x10000;

							if (mAnim == 7) {
								mAnim = 10;
								mFrame = Frame;
							}

							if (mAnim != 10)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(10, mFrame, mFrame, 2953);
						} else {
							int Frame = mAnimLeanAngle * 15 / MaxLeanAngle;

							if (Frame > 15)
								Frame = 15;

							mAnimSpeed = 0x10000;

							if (mAnim == 6) {
								mAnim = 9;
								mFrame = Frame;
							}

							if (mAnim != 9)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(9, mFrame, mFrame, 2968);
						}
					} else {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 15) / MaxLeanAngle;

							if (Frame > 15)
								Frame = 15;

							mAnimSpeed = 0x10000;

							if (mAnim == 6) {
								mAnim = 9;
								mFrame = Frame;
							}

							if (mAnim != 9)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(9, mFrame, mFrame, 2988);
						} else {
							int Frame = mAnimLeanAngle * 12 / MaxLeanAngle;

							if (Frame > 12)
								Frame = 12;

							mAnimSpeed = 0x10000;

							if (mAnim == 7) {
								mAnim = 10;
								mFrame = Frame;
							}

							if (mAnim != 10)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(10, mFrame, mFrame, 3003);
						}
					}
			}
		}
	}

	if (!mAnimLeanAngle) {
		switch (mAnim) {
			case 6:
			case 7:
				mAnimSpeed = 0x10000;
				Debug_RunAnim(0, 3017);
				break;
			case 9:
			case 10:
				mAnimSpeed = 0x10000;
				Debug_RunAnim3(8, 19, 26, 19, 3024);
				break;
		}
	}

	mPitchAngle = 0;

	if (mPhysicsState == 1 && !mpPad->Flip.Pressed && !mpPad->Grab.Pressed) {
		if (mpPad->Up.Pressed || mAY < -40)
			mPitchAngle = -((TimeScale * 20) >> 8);

		if (mpPad->Down.Pressed || mAY > 40)
			mPitchAngle = (TimeScale * 20) >> 8;
	}
}

/* AlignHeadingOnRail__6CBruce (0x80033108) */
/* Stands the skater upright on a rail: up is straight up, right = front x up
   (normalised), up = right x front, then rebuilds the rotation matrix. */
void CBruce::AlignHeadingOnRail() {
	mUp.vx = 0;
	mUp.vy = -4096;
	mUp.vz = 0;
	geom_cross(&mFront, &mUp, &mRight, 12);
	M3dUtils_NormaliseVector(&mRight);
	geom_cross(&mRight, &mFront, &mUp, 12);
	RotMatrixFromFRUVectors();
}

/* DoOnRailPhysics__6CBruce (0x800331e4) */
/* Demo twin (fuzzy), locals from its MAIN.SYM. Grinding: settles the grind
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

	if (mAnimFinished) {
		switch (mAnim) {
			case 41:
			case 68:
				if (Stats[mSkaterIndex].ComboID == 0 || Stats[mSkaterIndex].ComboID == 0x6000)
					Debug_RunAnim(41, 3104);
				else
					Debug_RunAnim1(41, 5, 3106);

				break;
			case 40:
			case 67:
				if (Stats[mSkaterIndex].ComboID == 0 || Stats[mSkaterIndex].ComboID == 0x6000)
					Debug_RunAnim(40, 3112);
				else
					Debug_RunAnim1(40, 3, 3114);

				break;
			case 70:
				Debug_RunAnim(42, 3120);
				break;
			case 17:
				if (mFrame >= 16)
					Debug_RunAnim(39, 3125);
				else
					Debug_RunAnim(37, 3129);

				break;
			case 37:
				Debug_RunAnim(38, 3133);
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
				switch (Stats[mSkaterIndex].ComboID) {
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
				Debug_RunAnim(mAnim, 3179);
				break;
		}
	}

	Vibrate(2, 0, 1);

	if (!m2EC) {
		switch (mAnim) {
			case 62:
			case 63:
			case 64:
				switch (Stats[mSkaterIndex].ComboID) {
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
				Bit_SetSparkTrajectoryCone(Cone);

				UseCacheForBits = 0;
				Pos.vx = Pos.vz = 0;
				Pos.vy = 0x19000;

				for (i = 0; i < 9; i++) {
					new CGrindSpark(this, Pos, -40, 4096, 5);
				}

				UseCacheForBits = 1;
			}
		}
	}
NoSparks:
	mAcc.Set(0, 0, 0);
	CVector Diff = mPos - mLastScorePos;

	if (Diff.Length() > 30) {
		Trick_HoldTrick(Trick_RepeatScore(mGrindTrick + 66, DAT_800a7edc[mGrindTrick]) / 5);

		mLastScorePos = mPos;
	}

	CVector Gravity;

	Gravity.Set(0, 13000, 0);

	do {
		int AccMagnitude;

		NextSegment = Rail_NextNode(Trig_GetLinksPointer(mRailNode));
		Trig_GetPosition(SegStart, mRailNode);
		Trig_GetPosition(SegEnd, NextSegment);

		SegDir = SegEnd - SegStart;
		M3dUtils_NormaliseVector(&SegDir);

		Speed = Mips_CVecDotProd(&SegDir, &mVel);

		if (FirstTimeThroughLoop) {
			FirstTimeThroughLoop = 0;
			mAcc += Gravity;

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

		AccMagnitude = Mips_CVecDotProd(&SegDir, &mAcc);
		mAcc.vx = Mips_FixMul(SegDir.vx, AccMagnitude);
		mAcc.vy = Mips_FixMul(SegDir.vy, AccMagnitude);
		mAcc.vz = Mips_FixMul(SegDir.vz, AccMagnitude);
		mVel.vx = Mips_FixMul(SegDir.vx, Speed);
		mVel.vy = Mips_FixMul(SegDir.vy, Speed);
		mVel.vz = Mips_FixMul(SegDir.vz, Speed);
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
			mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
			RemainingSpeed = 0;
		} else if (t < 0) {
			int PreviousSegment = -1;
			int n;

			for (n = mRailStart; n != mRailNode; n = Rail_NextNode(Trig_GetLinksPointer(n)))
				PreviousSegment = n;

			if (PreviousSegment == -1) {
				StateTransition(PHYSICS_IN_AIR, 3402);

				m70C = 5;
				mAnimSpeed = 0x10000;
				Debug_RunAnim(14, 3405);

				RemainingSpeed = 0;
				m6DC = 0;
				mStickToRail = 0;
				m740 = 1;
				mDoingATrick = 0;

				if (m210 == 1 && !m2EC)
					SFX_PlayX(SFX_RAIL_OFF, 0x2000, 0);
			} else {
				int NextSegment2 = Rail_NextNode(Trig_GetLinksPointer(PreviousSegment));
				CVector SegStart2;
				CVector SegEnd2;
				CVector SegDir2;

				Trig_GetPosition(SegStart2, PreviousSegment);
				Trig_GetPosition(SegEnd2, NextSegment2);

				SegDir2 = SegEnd2 - SegStart2;
				M3dUtils_NormaliseVector(&SegDir2);

				if (Mips_CVecDotProd(&SegDir, &SegDir2) < 2633) {
					StateTransition(PHYSICS_IN_AIR, 3440);

					m70C = 5;
					mAnimSpeed = 0x10000;
					Debug_RunAnim(14, 3443);

					RemainingSpeed = 0;
					m6DC = 0;
					mStickToRail = 0;
					m740 = 1;
					mDoingATrick = 0;

					if (m210 == 1 && !m2EC)
						SFX_PlayX(SFX_RAIL_OFF, 0x2000, 0);
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
				StateTransition(PHYSICS_IN_AIR, 3477);

				m70C = 5;
				mAnimSpeed = 0x10000;
				Debug_RunAnim(14, 3480);

				RemainingSpeed = 0;
				m6DC = 0;
				mStickToRail = 0;
				m740 = OffRail;
				mDoingATrick = 0;

				if (m210 == 1 && !m2EC)
					SFX_PlayX(SFX_RAIL_OFF, 0x2000, 0);
			} else {
				int NextSegment2 = Rail_NextNode(Trig_GetLinksPointer(NextSegment));
				CVector SegStart2;
				CVector SegEnd2;
				CVector SegDir2;

				Trig_GetPosition(SegStart2, NextSegment);
				Trig_GetPosition(SegEnd2, NextSegment2);

				SegDir2 = SegEnd2 - SegStart2;
				M3dUtils_NormaliseVector(&SegDir2);

				if (Mips_CVecDotProd(&SegDir, &SegDir2) < 2633) {
					StateTransition(PHYSICS_IN_AIR, 3517);

					m70C = 5;
					mAnimSpeed = 0x10000;
					Debug_RunAnim(14, 3520);

					RemainingSpeed = 0;
					m6DC = 0;
					mStickToRail = 0;
					m740 = OffRail;
					mDoingATrick = 0;

					if (m210 == 1 && !m2EC)
						SFX_PlayX(SFX_RAIL_OFF, 0x2000, 0);
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

	if (!mpPad->Left.Pressed && mAX >= -40 && !mpPad->Right.Pressed && mAX <= 40)
		m204 = 1;

	if ((mpPad->Left.Pressed || mAX < -40) && m204)
		mBalanceVel -= 20;
	else if ((mpPad->Right.Pressed || mAX > 40) && m204)
		mBalanceVel += 20;
	else if (ABS(mBalanceVel) < 20 - GetStat(3))
		mBalanceVel = Rnd(80 - GetStat(3) * 3) - (80 - GetStat(3) * 3) / 2;

	if (ABS(mBalance) > 4000) {
		Bail(0);

		mRailBails++;
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

		RotMatrixYXZ(&RotAngle, &Rail_RotMatrix);
		MulMatrix(&mRotMatrix, &Rail_RotMatrix);
		FRUVectorsFromRotMatrix();
	}
}

/* SkateOffGround__6CBruce (0x8003434c) */
/* Rolling off an edge into the air: from an invisible surface just changes
   state; otherwise changes state, takes off (Trick_TakeOff), clears the spin tally and trick flag, sets m740 and, unless
   already in the air (mInAir), resets the anim speed and runs the fall anim (26 or 27 by mManIsCrouched). */
void CBruce::SkateOffGround() {
	int v;

	if (mPhysicsState == PHYSICS_ON_INVISIBLE) {
		StateTransition(PHYSICS_IN_AIR, 3707);
	} else {
		StateTransition(PHYSICS_IN_AIR, 3713);
		Trick_TakeOff();

		v = mInAir;
		mTallyAngles = 0;
		mNum180Spins = 0;
		mDoingATrick = 0;
		m740 = 1;

		if (v == 0) {
			mAnimSpeed = 0x10000;

			if (mManIsCrouched)
				Debug_RunAnim(26, 3730);
			else
				Debug_RunAnim(27, 3732);
		}
	}
}

/* CheckSide__6CBrucei (0x800343f0) */
/* Side collision probe: casts a line from 30 units up the skater along mRight
   (`dir` = -1/+1 picks the side; 5 units per step, 35 in the air). If it hits a wall-like face
   (|normal . mUp| < 1023), the skater is put back at mOldPos; a fast hit roughly head-on (normal .
   mFront >= 2897, on the ground, no trick) plays the wall-bump anim and grunts. The heading and
   velocity are reflected off the face, then a second probe along the normal moves the skater to
   its end if clear (returns 1) or reports 0. */
int CBruce::CheckSide(int a) {
	SLineInfo li;
	CVector d;
	CVector c;
	int s1, q0, q1, q2, q3, q4, q5;

	li.StartCoords.vx = mPos.vx + mUp.vx * 30;
	li.StartCoords.vy = mPos.vy + mUp.vy * 30;
	li.StartCoords.vz = mPos.vz + mUp.vz * 30;
	ST(li.EndCoords.vx, li.StartCoords.vx, q0, a * mRight.vx)
	ST(li.EndCoords.vy, li.StartCoords.vy, q1, a * mRight.vy)
	ST(li.EndCoords.vz, li.StartCoords.vz, q2, a * mRight.vz)
	M3dColij_InitLineInfo(&li);
	M3dZone_LineToItemNotSCached(&li, 1);
	GetFaceFlags(&li);

	if (li.pItem) {
		d.vx = li.Normal.vx;
		d.vy = li.Normal.vy;
		d.vz = li.Normal.vz;

		if ((unsigned)(Mips_CVecDotProd(&d, &mUp) + 1023) < 2047) {
			s1 = 0;
			mPos = mOldPos;
			c = mVel;

			if (mAnim != 12) {
				if (c.Length() >= 7) {
					if (!mInAir && !m368) {
						CVector e;

						e.vx = li.Normal.vx;
						e.vy = li.Normal.vy;
						e.vz = li.Normal.vz;

						if (Mips_CVecDotProd(&e, &mFront) >= 2897) {
							mAnimSpeed = 0x10000;
							Debug_RunAnim(12, 3787);

							s1 = 200;
							SFX_Grunt(this);
						}
					}
				}
			}

			ReflectHeadingVelocityInPlane(li.Normal, s1);

			li.StartCoords.vx = mPos.vx;
			li.StartCoords.vy = mPos.vy;
			li.StartCoords.vz = mPos.vz;
			ST(li.EndCoords.vx, li.StartCoords.vx, q3, li.Normal.vx)
			ST(li.EndCoords.vy, li.StartCoords.vy, q4, li.Normal.vy)
			ST(li.EndCoords.vz, li.StartCoords.vz, q5, li.Normal.vz)
			M3dColij_InitLineInfo(&li);
			M3dZone_LineToItemNotSCached(&li, 1);
			GetFaceFlags(&li);

			if (li.pItem)
				return 0;

			mPos.vx = li.EndCoords.vx;
			mPos.vy = li.EndCoords.vy;
			mPos.vz = li.EndCoords.vz;
			return 1;
		}
	}

	return 0;
}

/* DoOnGroundPhysics__6CBruce (0x800347a8) */
/* Demo twin, locals from its MAIN.SYM. One function: functions.tsv splits it into
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

	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

	if ((mTerrain == 5 || mTerrain == 6 || mTerrain == 3) && BitCount < 200) {
		SetAnglesFromHeading();
		Bit_SetSparkTrajectory(mAngles);
		CSVector Cone;

		Cone.Set(512, 4096, 0);
		Bit_SetSparkTrajectoryCone(Cone);
		Bit_SetSparkSize(1);

		if (Speed > 0x10000) {
			int i;

			switch (mTerrain) {
				case 5:
					Bit_SetSparkRGB(2, 160, 32);
					break;
				case 6:
					Bit_SetSparkRGB(160, 160, 160);
					break;
				case 3:
					Bit_SetSparkRGB(128, 64, 32);
					break;
			}

			Bit_SetSparkFadeRGB(0, 0, 0);

			UseCacheForBits = 0;

			for (i = 0; i < 6; i++) {
				if (Mem_GetFree(1) > 10000) {
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
	((void (*)(int))M3dMaths_SquareRoot0)(Mips_CVecDotProd(&vMovement, &vMovement));
	TurnAngle = mLeanAngle >> 12;
	mFallLineTurn = 0;

	if (!m720 && !mTapTurn && m1FC && !mNoSpin) {
		if (!TurnAngle) {
			AlignToFallLine(40);
		} else if (mpPad->Flip.Pressed || mpPad->Grab.Pressed) {
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

	Mips_FixMul(TurnAngle, 3072);
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
		Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&vMovement, &vMovement)) << 6;

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
		M3dZone_LineToItemNotSCached(&line_info, 1);
		GetFaceFlags(&line_info);

		if (line_info.pItem) {
			if (!Face_Skateable && !Face_WallRide && !Face_Air) {
				int TurnAway;

				mPos = mOldPos;
				TurnAway = 0;

				if (mAnim != 12 && !mInAir && !m368) {
					CVector WallFloorNormal;

					SetVector(&WallFloorNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) if (Mips_CVecDotProd(&WallFloorNormal, &mFront) >
					                                   2896) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(12, 4120);

						TurnAway = 200;
						SFX_Grunt(this);
					}
				}

				ReflectHeadingVelocityInPlane(line_info.Normal, TurnAway);
			} else if (line_info.pItem) {
				if (!Face_Air || Face_Visible) {
					CVector WallFloorNormal;
					int UpDot;

					SetVector(&WallFloorNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) UpDot = Mips_CVecDotProd(&WallFloorNormal, &mUp);

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
								          line_info.Normal.vz)
								geom_cross(&mFront, &mUp, &mRight, 12);
								RotMatrixFromFRUVectors();
							}

							StateTransition(PHYSICS_ON_GROUND, 4193);
						} else {
							mPos = mOldPos;
							CVector OldVel = mVel;

							if (mAnim != 12 && OldVel.Length() >= 7 && !mInAir && !m368) {
								CVector WallFloorNormal;

								SetVector(
								    &WallFloorNormal, line_info.Normal.vx, line_info.Normal.vy,
								    line_info.Normal.vz) if (Mips_CVecDotProd(&WallFloorNormal,
									                                          &mFront) > 2896) {
									mAnimSpeed = 0x10000;
									Debug_RunAnim(12, 4219);
									SFX_Grunt(this);

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

	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

	if (mMaxSpeed < Speed)
		m304 = 1;
	else
		m304 = 0;

	CVector Old = mPos;

	if (CheckSide(-1)) {
		if (CheckSide(1))
			mPos = Old;
	} else if (CheckSide(1)) {
		if (CheckSide(-1))
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
	M3dZone_LineToItemNotSCached(&line_info, 1);
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

				if (Mips_CVecDotProd(&Move, &Move) < 4096)
					mPos = Orig;

				if (Face_Air) {
					if (!Face_Visible && m1F8 && XFrames - m1F8 >= 5 && !mInAir &&
					    mPhysicsState != 2) {
						SVECTOR N;

						N = line_info.Normal;
						mVel.vx -= Mips_FixMul(N.vx, Speed / 4);
						mVel.vz -= Mips_FixMul(N.vz, Speed / 4);
						mVel.vy /= 2;

						if (!mPhysicsState) {
							mNum180Spins = 0;
							mTallyAngles = 0;
						}

						StateTransition(PHYSICS_IN_AIR, 4466);
						Trick_TakeOff();

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
							Trick_TakeOff();

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
						    SetVector(&LastNormal, mNormal.vx, mNormal.vy, mNormal.vz) Dot =
						        Mips_CVecDotProd(&LastNormal, &NewNormal);

						if (Mips_CVecDotProd(&mFront, &NewNormal) < 0 && mVel.vy <= 0 && Dot > 0 &&
						    Dot < 3000)
							SkateOffGround();
					} else {
						StateTransition(PHYSICS_ON_GROUND, 4551);
					}

					if (!OnInvisible) {
						mpWallItem = line_info.pItem;
						mWallHandle = Mem_GetHandle(PSXRegion[line_info.pItem->Region].pPSX);
					}
				} else {
					SkateOffGround();
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
				SkateOffGround();
			}
		} else {
			SkateOffGround();
		}
	} else {
		SkateOffGround();
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

		dot = Mips_CVecDotProd(&mVel, &mRight);
		SideVelocity.vx = Mips_FixMul(dot, mRight.vx);
		SideVelocity.vy = Mips_FixMul(dot, mRight.vy);
		SideVelocity.vz = Mips_FixMul(dot, mRight.vz);
		mVel -= SideVelocity;
		dot = Mips_CVecDotProd(&mVel, &mFront);
		ForwardsVelocity.vx = Mips_FixMul(dot, mFront.vx);
		ForwardsVelocity.vy = Mips_FixMul(dot, mFront.vy);
		ForwardsVelocity.vz = Mips_FixMul(dot, mFront.vz);

		if (!m304) {
			if (AutoKickEnabled) {
				mAcc.vx -= Mips_FixMul(8, ForwardsVelocity.vx);
				mAcc.vy -= Mips_FixMul(8, ForwardsVelocity.vy);
				mAcc.vz -= Mips_FixMul(8, ForwardsVelocity.vz);
			}
		} else {
			mAcc.vx -= Mips_FixMul(82, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul(82, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul(82, ForwardsVelocity.vz);
		}

		if (mTerrain == 5) {
			mAcc.vx -= Mips_FixMul(120, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul(120, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul(120, ForwardsVelocity.vz);
		}

		if (mTerrain == 6) {
			mAcc.vx -= Mips_FixMul(120, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul(120, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul(120, ForwardsVelocity.vz);
		}

		if (mTerrain == 3) {
			mAcc.vx -= Mips_FixMul(180, ForwardsVelocity.vx);
			mAcc.vy -= Mips_FixMul(180, ForwardsVelocity.vy);
			mAcc.vz -= Mips_FixMul(180, ForwardsVelocity.vz);
		}
	}
}

/* arccos__Fi (0x80035dc8) */
/* A cheat behind a math name: given the magic value, the secret skater (Stats[10], 0x800a5244)
   and three of his tricks are swapped for the alternates. */
int arccos(int a) {
	if (a == 0x2253c038) {
		Stats[10] = DAT_800a7f40;
		Group[63] = DAT_800a8094;
		Group[64] = DAT_800a80c0;
		Group[65] = DAT_800a80ec;
	}

	return 0;
}

/* HandleLeaningBail__6CBruceG7SVECTOR (0x80035f30) */
/* On the ground, outside a manual: if the skater leans too far from the surface
   normal `n` (mUp . n < 2000) and is not upside down, bails, runs the leaning-bail anim, swaps the
   front/up vectors (front = -up, up = old front) and throws the skater backwards at speed 10. */
void CBruce::HandleLeaningBail(SVECTOR s) {
	CVector v;
	CVector t;

	if (mInAir == 0) {
		if (mPhysicsState != 2) {
			v.vx = s.vx;
			v.vy = s.vy;
			v.vz = s.vz;

			if (Mips_CVecDotProd(&mUp, &v) < 2000 && mUp.vy >= -1999) {
				Bail(0);

				mAnimSpeed = 0x10000;
				Debug_RunAnim(56, 4860);

				m24C = 9;
				t = mFront;
				mFront = -mUp;
				mUp = t;
				RotMatrixFromFRUVectors();

				mVel = -mFront * 10;
				mVel.vy = 0;
			}
		}
	}
}

/* DoInAirPhysics__6CBruce (0x8003609c) */
/* Demo twin, locals from its MAIN.SYM. In the air: moves the skater by the
   frame-scaled velocity and kick, applies the surface spin, turns by the spin input, keeps the
   skater upright, then sweeps the movement against the world. Hitting a wall that is steep
   enough at a shallow angle starts a wall ride (or bails when going too slowly for the wall);
   otherwise a wall knocks the skater back down and a floor lands them (grind / manual / bail
   checks). No hit: the gravity is added to the kick.
   Levers: unused CVector locals keep retail's frame slots; WallRight is declared just before its
   GTE use (so the by-value SVECTOR argument temps share its slot, as in retail); the unused
   SquareRoot0 call goes through a void cast and the libgte calls return MATRIX * / VECTOR *
   (v0/v1 choice); the m368 trick-limit test is if / else-if with a Trick index local; the
   landing sound is a switch with a call per arm; m200 = 15 is stored twice around the mPos
   update (the demo's MoveOriginTo call sat between the two stores). */
void CBruce::DoInAirPhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector u3, u4;
	int Speed;
	int TurnAngle;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	SLineInfo line_info;
	CVector Gravity;

	Gravity = mGravity;
	mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
	Platform_HandleBounce();

	if (mPitchAngle) {
		VECTOR zero, v, v2, v3;

		zero.vz = 0;
		zero.vy = 0;
		zero.vx = 0;
		TransMatrix(&mRotMatrix, &zero);
		TransMatrix(&SurfaceRotMatrix, &zero);

		v.vx = 0;
		v.vy = 0x46000;
		v.vz = 0;
		ApplyMatrixLV(&mRotMatrix, &v, &v2);

		RotAngle.vz = 0;
		RotAngle.vy = 0;
		RotAngle.vx = mPitchAngle & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
		ApplyMatrixLV(&mRotMatrix, &v, &v3);

		mPos.vx = mPos.vx + v2.vx - v3.vx;
		mPos.vy = mPos.vy + v2.vy - v3.vy;
		mPos.vz = mPos.vz + v2.vz - v3.vz;
	}

	vMovement = mPos - mOldPos;
	((void (*)(int))M3dMaths_SquareRoot0)(Mips_CVecDotProd(&vMovement, &vMovement));
	TurnAngle = mLeanAngle >> 12;
	Mips_FixMul(TurnAngle, 3072);

	if (TurnAngle && m814 == ZeroVector) {
		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = TurnAngle & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);

		mTallyAngles += TurnAngle;

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
	}

	if (mPhysicsState != 2)
		UprightSideways();

	SetVector(&line_info.StartCoords, mOldPos.vx, mOldPos.vy, mOldPos.vz)
	    SetVector(&line_info.EndCoords, mPos.vx, mPos.vy, mPos.vz) vMovement = mPos - mOldPos;
	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&vMovement, &vMovement)) << 6;

	if (Speed < 0x2e000) {
		if (Speed < 2048)
			Speed = 2048;

		line_info.EndCoords.vx = mOldPos.vx + ((vMovement.vx * 46 / Speed) << 12);
		line_info.EndCoords.vy = mOldPos.vy + ((vMovement.vy * 46 / Speed) << 12);
		line_info.EndCoords.vz = mOldPos.vz + ((vMovement.vz * 46 / Speed) << 12);
	}

	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached(&line_info, 1);
	GetFaceFlags(&line_info);

	if (Face_Air && !Face_Visible && mPhysicsState != 3) {
		BulletCollisionCheck = 1;
		M3dColij_InitLineInfo(&line_info);
		M3dZone_LineToItemNotSCached(&line_info, 1);
		GetFaceFlags(&line_info);

		BulletCollisionCheck = 0;
	}

	if (line_info.pItem) {
		if ((!Face_Skateable || (Face_Air && Face_Visible)) && Face_WallRide && !mInAir &&
		    (mpPad->Grind.Pressed || mpPad->Grind.ReleasedTime < 20)) {
			CVector UnitVel;
			CVector FaceNormal;
			int VelNormal;
			int Steep = line_info.Normal.vy >= 3278;

			UnitVel = mVel;
			UnitVel.vy = 0;
			M3dUtils_NormaliseVector(&UnitVel);

			FaceNormal.vx = line_info.Normal.vx;
			FaceNormal.vy = 0;
			FaceNormal.vz = line_info.Normal.vz;
			M3dUtils_NormaliseVector(&FaceNormal);

			VelNormal = Mips_CVecDotProd(&FaceNormal, &UnitVel);

			if (!Steep && ((VelNormal ^ (VelNormal >> 31)) - (VelNormal >> 31)) < 3547 &&
			    XFrames - m208 >= 41) {
				int Limit = 10000;
				int Trick = m368 & 255;

				if (m368 & 0x4000)
					Limit = mNumFrames - Group[Trick].MinFrame;
				else if (m368 & 0x2000)
					Limit = mNumFrames - Group[Trick].MinFrame;

				if (mDoingATrick && !m360 && mFrame < Limit) {
					Bail(0);

					mTrickBails++;
				} else
					m360 = 1;

				if (!mInAir) {
					FrigDarkenObject();
					StateTransition(PHYSICS_IN_WALLRIDE, 5197);

					m208 = XFrames;
					mPos.vx = line_info.Position.vx + line_info.Normal.vx * 30;
					mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
					mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;
					ProjectCVectorToPlane(&mFront, line_info.Normal);
					M3dUtils_NormaliseVector(&mFront);
					SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
					    geom_cross(&mFront, &mUp, &mRight, 12);
					RotMatrixFromFRUVectors();
					FlipToVelocity();

					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
					    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz) mSameNormal = 0;
					ProjectCVectorToPlane(&mVel, line_info.Normal);
					CVector WallRight;

					geom_cross(&WorldUp, &mUp, &WallRight, 12);
					RotMatrixFromFRUVectors();

					mAnimSpeed = 0x10000;

					if (mExtraFlags & 2) {
						if (Mips_CVecDotProd(&WallRight, &mVel) >= 0)
							Debug_RunAnim(35, 5238);
						else
							Debug_RunAnim(36, 5242);
					} else {
						if (Mips_CVecDotProd(&WallRight, &mVel) >= 0)
							Debug_RunAnim(36, 5249);
						else
							Debug_RunAnim(35, 5253);
					}
				} else {
					mPos = mOldPos;
				}
			} else {
				mPos = mOldPos;
				printf("...............New Bail..............\n");
				Bail(0);
			}
		} else if (!Face_Skateable && (!Face_Air || mPhysicsState != 3)) {
			switch (Face_Terrain) {
				case 7:
					SFX_PlayX(SFX_HIT_TERRAIN7, 0x2000, 0);
					break;
				case 9:
					if (GLevel == 8)
						SFX_PlayX(SFX_ROSWELL_CLANG, 0x2000, 0);
					else
						SFX_PlayX(SFX_CLANG, 0x2000, 0);

					break;
			}

			mNormal.vx = 0;
			mNormal.vy = -4096;
			mNormal.vz = 0;
			SetVector(&mOldNormal, 0, -4096, 0) mSameNormal = 0;
			OrientToNormal();

			m200 = 15;
			mPos.vx = line_info.Position.vx + line_info.Normal.vx;
			mPos.vy = line_info.Position.vy + line_info.Normal.vy;
			mPos.vz = line_info.Position.vz + line_info.Normal.vz;
			m200 = 15;
			StateTransition(PHYSICS_IN_AIR, 5363);

			m740 = 1;
			ReflectHeadingVelocityInPlane(line_info.Normal, 0);

			mVel.vx += line_info.Normal.vx / 4;
			mVel.vy += line_info.Normal.vy / 4;
			mVel.vz += line_info.Normal.vz / 4;

			if (!Rnd(3)) {
				mVel.vx += Rnd(16384) - 8192;
				mVel.vz += Rnd(16384) - 8192;
			}
		} else {
			mPos.vx = line_info.Position.vx + line_info.Normal.vx * 30;
			mPos.vy = line_info.Position.vy + line_info.Normal.vy * 30;
			mPos.vz = line_info.Position.vz + line_info.Normal.vz * 30;

			if (!mInAir && Face_Air) {
				if (m764 && (!m1F8 || XFrames - m1F8 < 5 || Face_Visible)) {
					StateTransition(PHYSICS_ON_INVISIBLE, 5428);
				} else {
					StateTransition(PHYSICS_IN_AIR, 5435);

					if (mpPad->Up.Pressed || mAY < -40) {
						SVECTOR N;
						int Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

						N = line_info.Normal;
						mVel.vx -= Mips_FixMul(N.vx, Spd / 4);
						mVel.vz -= Mips_FixMul(N.vz, Spd / 4);
					}

					m768 = 0;

					if (!m740 || !mDoingATrick) {
						Trick_TakeOff();

						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 5457);

						m740 = 1;
					}
				}
			} else if (mPhysicsState != 3 || XFrames - m2FC >= 31) {
				StateTransition(PHYSICS_ON_GROUND, 5487);

				mTerrain = Face_Terrain;

				if (!mInAir) {
					HandleBailPerpendicular(line_info.Normal);
					HandleLeaningBail(line_info.Normal);
					HandleBailOnLanding(5496);
				}
			}

			m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;

			if (mPhysicsState != 1) {
				ProjectCVectorToPlane(&mVel, line_info.Normal);

				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) mSameNormal = 0;
				OrientToNormal();
			}
		}
	} else {
		mAcc += Gravity;
	}
}

/* DoFootPlantPhysics__6CBruce (0x80036f7c) */
/* Demo twin (no locals in MAIN.SYM). Foot plant: fires the trick, sets the
   plant timer from the skater's stats, flips the velocity back along -mFront (keeping speed),
   nudges it forward and kicks the skater upward by a stat-derived amount.
   GetStat is a macro, not an inline: gcc 2.x fold() distributes the enclosing arithmetic into the
   ?: arms (and hoists the save_expr'd operands), which is what produces retail's jump-threaded
   override tests and the per-arm sums. Operand order (Stats + (flag ? 3 : 0)) + Bonus fixes the
   load order (base, then bonus byte). */
void CBruce::DoFootPlantPhysics() {
	Trick_Trick(65, "FOOTPLANT", 100, 0);

	mJumpStrength = 15 - (GetStat(0) + GetStat(2)) / 2;
	StateTransition(PHYSICS_IN_AIR, 5587);

	m740 = 1;
	mVel = -mFront * mVel.Length();
	mVel.vx += mFront.vx * 5;
	mVel.vz += mFront.vz * 5;
	mVel.vy -=
	    (mJumpStrength * (((32 - (10 - GetStat(0)) / 2) + ((10 - GetStat(0)) / 3 - 18)) << 10) /
		     (15 - (GetStat(0) + GetStat(2)) / 2) +
		 ((32 - (10 - GetStat(0)) / 2) << 10)) *
	    3;
}

/* DoHandPlantPhysics__6CBruce (0x80037460) */
/* Handplant: only on the handplant anims (else bail). Holding triangle in
   frames 20..24 ping-pongs the hold (too long bails); at the end, launches back up off the wall
   into the air, flipping the skater if it was the second handplant anim. */
void CBruce::DoHandPlantPhysics() {
	m70C = 5;

	if (mAnim != 0x22 && mAnim != 0x3f) {
		/* The retail call leaves the limit argument unset; Bail ignores it. */
		Bail(0);
		return;
	}

	if (mAnim < 0x3e && mFrame >= 20 && mFrame < 25) {
		int Hold = m248;

		if (Hold >= 0) {
			if (mpPad->Grind.Pressed) {
				mFrame = PingPong(20, 22, 15);
				Trick_HoldTrick(20);

				m248++;

				if (m248 >= 21) {
					Bail(0);
					Debug_RunAnim(0x38, 0x1602);

					m24C = 9;
					return;
				}
			} else {
				m248 = -Hold;
			}
		}
	}

	if (mAnimFinished != 0 || (mAnim < 0x3e && mFrame >= 31)) {
		int Hold = m248;

		m740 = 0;
		mVel.vy = (40 - Hold * 3) << 12;
		StateTransition(PHYSICS_ON_GROUND, 0x1614);

		m364 = 1;
		Trick_Land();

		mPos.vx += mUp.vx * 30;
		mPos.vy += mUp.vy * 30;
		mPos.vz += mUp.vz * 30;

		if (mAnim >= 0x3e) {
			mFlipPending = 0;
			mFlipAxesPending = 0;
			mExtraFlags ^= 2;
			FlipAxis(mRotMatrix.m[0][2], mFront.vx);
			FlipAxis(mRotMatrix.m[1][2], mFront.vy);
			FlipAxis(mRotMatrix.m[2][2], mFront.vz);
			FlipAxis(mRotMatrix.m[0][0], mRight.vx);
			FlipAxis(mRotMatrix.m[1][0], mRight.vy);
			FlipAxis(mRotMatrix.m[2][0], mRight.vz);
			Debug_RunAnim(0, 0x162b);
		}
	}
}

/* DoWallRidePhysics__6CBruce (0x800376f4) */
/* Demo twin, locals from its MAIN.SYM. Wall ride: moves the skater by the
   frame-scaled velocity and kick, turns along the fall line, sweeps the movement against the
   world (snapping to the wall and re-orienting if it was hit), then probes along -mUp for the
   wall: on it, the skater is stuck to it and gravity projected into its plane is added; off it,
   the skater drops back into the air.
   Levers: TimeScale/1778 are plain `extern int` (one `la`/`lw` macro insn each, so the HIGH
   part isn't CSE'd into a saved reg); SetVector is a macro with block locals _x/_y/_z (as in the
   demo's MAIN.SYM blocks); `CheckDist` as a variable gives retail's `slt` with 116 in a register;
   the `Steep` int gives the slti/xori scc; the unused SquareRoot0 call goes through a void cast. */
void CBruce::DoWallRidePhysics() {
	CVector u0, u1, u2; /* unused: retail's frame keeps their slots */
	CVector vMovement;
	CVector InplaneGravity;
	CVector u3, u4, u5; /* unused, likewise */
	SVECTOR u6;
	int Speed;
	int TurnAngle;
	SLineInfo line_info;
	int OnInvisible;
	CVector Gravity;

	Gravity.vx = 0;
	Gravity.vy = 9000;
	Gravity.vz = 0;
	OnInvisible = mPhysicsState == 2;

	if (mAnimFinished) {
		if (mAnim == 36)
			mFrame = PingPong(7, 9, 15);
		else if (mAnim == 35)
			mFrame = PingPong(7, 9, 15);
	}

	mPos += ((mVel * TimeScale) >> 8) + ((mAcc * TimeScaleSquared) >> 8) / 2;
	vMovement = mPos - mOldPos;
	/* result unused; called as void so v0 isn't reserved (retail's register choice) */
	((void (*)(int))M3dMaths_SquareRoot0)(Mips_CVecDotProd(&vMovement, &vMovement));
	TurnAngle = 0;

	if (!m720 && !mTapTurn) {
		AlignToFallLine(4);

		TurnAngle = mFallLineTurn;
	}

	Mips_FixMul(TurnAngle, 3072);
	RotateHeadingAndVelocity((TurnAngle * TimeScale) >> 8, !OnInvisible, 0);

	SetVector(&line_info.StartCoords, mOldPos.vx, mOldPos.vy, mOldPos.vz)
	    SetVector(&line_info.EndCoords, mPos.vx, mPos.vy, mPos.vz) vMovement = mPos - mOldPos;

	if (vMovement != ZeroVector) {
		Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&vMovement, &vMovement)) << 6;

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
		M3dZone_LineToItemNotSCached(&line_info, 1);
		GetFaceFlags(&line_info);

		if (line_info.pItem) {
			if (Face_Visible || !Face_Air) {
				mPos.vx = line_info.Position.vx + mUp.vx * 30;
				mPos.vy = line_info.Position.vy + mUp.vy * 30;
				mPos.vz = line_info.Position.vz + mUp.vz * 30;

				if (!OnInvisible) {
					mpWallItem = line_info.pItem;
					mWallHandle = Mem_GetHandle(PSXRegion[line_info.pItem->Region].pPSX);
				}

				if (*(int *)&line_info.Normal == *(int *)&mNormal &&
				    line_info.Normal.vz == mNormal.vz) {
					mSameNormal++;
				} else {
					mSameNormal = 0;
					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
				}

				OrientToNormal();

				if (Face_Skateable) {
					StateTransition(PHYSICS_ON_GROUND, 5840);
					ProjectCVectorToPlane(&mFront, line_info.Normal);
					M3dUtils_NormaliseVector(&mFront);
					SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
					geom_cross(&mFront, &mUp, &mRight, 12);
					RotMatrixFromFRUVectors();

					mSameNormal = 0;
					SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz)
					    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
						          line_info.Normal.vz)
					        ProjectCVectorToPlane(&mVel, line_info.Normal);
				}
			}
		}
	}

	if (mMaxSpeed < M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6)
		m304 = 1;
	else
		m304 = 0;

	FRUVectorsFromRotMatrix();

	line_info.StartCoords.vx = mPos.vx + mUp.vx * 70;
	line_info.StartCoords.vy = mPos.vy + mUp.vy * 70;
	line_info.StartCoords.vz = mPos.vz + mUp.vz * 70;
	line_info.EndCoords.vx = line_info.StartCoords.vx - mUp.vx * 128;
	line_info.EndCoords.vy = line_info.StartCoords.vy - mUp.vy * 128;
	line_info.EndCoords.vz = line_info.StartCoords.vz - mUp.vz * 128;
	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItemNotSCached(&line_info, 1);
	GetFaceFlags(&line_info);

	if (line_info.pItem) {
		int d = line_info.Distance;
		int CheckDist = 116;

		if (d <= CheckDist) {
			m768 = ((SFace *)line_info.pFace)->Flags2 & 0x20;
			mPos.vx = line_info.Position.vx + mUp.vx * 30;
			mPos.vy = line_info.Position.vy + mUp.vy * 30;
			mPos.vz = line_info.Position.vz + mUp.vz * 30;

			if (!OnInvisible) {
				mpWallItem = line_info.pItem;
				mWallHandle = Mem_GetHandle(PSXRegion[line_info.pItem->Region].pPSX);
			}

			if (*(int *)&line_info.Normal == *(int *)&mNormal &&
			    line_info.Normal.vz == mNormal.vz) {
				mSameNormal++;
			} else {
				mSameNormal = 0;
				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
			}

			OrientToNormal();

			if (Face_Skateable) {
				StateTransition(PHYSICS_ON_GROUND, 5949);
				ProjectCVectorToPlane(&mFront, line_info.Normal);
				M3dUtils_NormaliseVector(&mFront);
				SetVector(&mUp, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				geom_cross(&mFront, &mUp, &mRight, 12);
				RotMatrixFromFRUVectors();

				mSameNormal = 0;
				SetVector(&mNormal, line_info.Normal.vx, line_info.Normal.vy, line_info.Normal.vz)
				    SetVector(&mOldNormal, line_info.Normal.vx, line_info.Normal.vy,
					          line_info.Normal.vz) ProjectCVectorToPlane(&mVel, line_info.Normal);
			} else {
				int Steep = line_info.Normal.vy >= 3278;

				if (Steep) {
					mPos = mOldPos;
					StateTransition(PHYSICS_IN_AIR, 5987);

					m740 = 1;
					mAcc += Gravity;
					ReflectHeadingVelocityInPlane(line_info.Normal, 200);
					SFX_Grunt(this);
				}
			}
		}

		InplaneGravity = Gravity;
		ProjectCVectorToPlane(&InplaneGravity, line_info.Normal);

		mAcc += InplaneGravity;
		mFallLine = InplaneGravity;
		mPolyNormal = line_info.Normal;
		CVectorRotateToPlane(&mVel, &line_info.Normal, &mVel);
	} else {
		StateTransition(PHYSICS_IN_AIR, 6040);
		Trick_TakeOff();

		mTallyAngles = 0;
		mNum180Spins = 0;
		mDoingATrick = 0;
		m740 = 1;

		if (!mInAir) {
			mAnimSpeed = 0x10000;

			if (mManIsCrouched)
				Debug_RunAnim(26, 6071);
			else
				Debug_RunAnim(27, 6073);
		}

		mAcc += Gravity;
	}
}

/* HandleJump__6CBruce (0x80038268) */
/* Demo twin, locals from its MAIN.SYM. Ollie handling: settles the crouch
   anims, plays the landing sound/vibration and scores the landing, counts the crouch (ollie
   charge) timer while on the ground, and on release launches the skater: off a rail, along the
   rail; off a wall-ride, away from the wall; the upward kick scales with the charge and the
   skater's stats. A quick double-tap within the window does a boneless / no-comply.
   GetStat is the shared stat macro (see dofootplant.cpp). Levers: SFX_PlayX (SFX_PlayX__FUisi) is
   declared void here (v0 free for the |mUp.vy| test); the two JumpBoost arms share their tail by
   cross-jumping, so the boost is a local added once; the early-out call is wrapped in
   do { } while (0) (keeps `sw mJumpPending` ahead of the argument setup). */
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
					SFX_PlayX((ESfx)(SFX_IMPACT_1 + Rnd(2)), 0x2000, 0);
					break;
				case 10:
				case 12:
					SFX_PlayX(SFX_LAND_TERRAIN10, 0x2000, 0);
					break;
				case 6:
					break;
				case 1:
				case 15:
				default:
					SFX_PlayX(SFX_LAND, 0x2000, 0);
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
				                 : "r"(mPanel.TotalScore())
				                 : "$3");
				int Bonus = mPanel.TotalScore() / 100;

				if (Bonus > 20)
					Bonus = 20;

				m340 = 30;
				m344 = 30;
				m33C += Bonus;
				Trick_Bonus(Bonus);
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
			m2F8 = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

			if (m6DC) {
				int a;

				m70C = 5;
				m6DC = 0;
				mStickToRail = 0;
				mDoingATrick = 0;
				mFront = mRailDir;
				AlignHeadingOnRail();

				Speed = Mips_CVecDotProd(&mRailDir, &mVel);
				mVel.vx = Mips_FixMul(mRailDir.vx, Speed);
				mVel.vy = Mips_FixMul(mRailDir.vy, Speed);
				mVel.vz = Mips_FixMul(mRailDir.vz, Speed);
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

				/* called as returning void: retail's register choice around it (README: callee
				   return types change register allocation) */
				((void (*)(uint, short, int))SFX_PlayX)(Sfx, 0x2000, 0);
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
						Trick_TakeOff();
						Trick_Trick(65, "FASTPLANT", 250, 0);

						mVel.vy -= 0x5000;
						Debug_RunAnim1(45, 16, 6508);
					} else {
						Trick_TakeOff();
						Trick_Trick(75, "NOLLIE", 200, 0);
						Debug_RunAnim(13, 6515);
					}

					m364 = 1;
				} else {
					Trick_TakeOff();
					Debug_RunAnim(4, 6523);
				}
			}

			m2FC = XFrames;
		}

		mJumpStrength = 0;
	}
}

/* HandleKick__6CBruce (0x800395b4) */
/* Demo twin; locals from its MAIN.SYM. Pushing off: settles the push anims,
   counts down the kick timer and, when on the ground (or with auto-kick enabled) and not
   blocked, starts a kick (target speed from the skater's stat 1) and, during the kick frames,
   sets the kick impulse along -mFront. GetStat is the shared stat macro (see dofootplant.cpp). */
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

	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;
	mVel2 = mVel;
	ProjectCVectorToPlane(&mVel2, mPolyNormal);

	x = Mips_CVecDotProd(&mFallLine, &mVel2);

	if (mpPad->Flip.Pressed || AutoKickEnabled) {
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

			Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

			if (mAnim == 3 || m918 == 2) {
				int KickVal = 4;

				if (mFrame >= 11 && m714) {
					m718 = 34;
					SFX_PlayX(SFX_KICK, 0x2000, 0);

					m714 = 0;
				}

				if ((unsigned short)(mFrame - 11) < 5) {
					if (mpPad->Flip.Pressed)
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

	if (mpPad->Flip.Pressed) {
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

	Speed = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

	if (mAnim == 3 || m918 == 2) {
		int KickVal = 4;

		if ((unsigned short)(mFrame - 11) < 5) {
			if (mpPad->Flip.Pressed)
				KickVal = 8;

			if (Speed < mKickSpeed && mPhysicsState != 2) {
				mAcc.vx = -mFront.vx * KickVal;
				mAcc.vy = -mFront.vy * KickVal;
				mAcc.vz = -mFront.vz * KickVal;
			}
		}
	}
}

/* RotateHeadingAndVelocity__6CBruceiii (0x80039cd0) */
/* C++: CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE match_sym.mjs ... "-O2 -fno-exceptions" */
void CBruce::RotateHeadingAndVelocity(int a, int c, int b) {
	MATRIX m;
	SVECTOR ang;
	MATRIX m3;
	MATRIX m2;
	CVector tmp;
	CVector r;
	int l0, l1;
	CVector *s1;

	if (a) {
		if (mInAir == 0) {
			if (!(m814 != ZeroVector)) {
				ang.vz = 0;
				ang.vx = 0;
				ang.vy = a & 4095;
				RotMatrixYXZ(&ang, &m);
				MulMatrix(&mRotMatrix, &m);
				FRUVectorsFromRotMatrix();

				if (c) {
					s1 = &mVel;
					l0 = M3dMaths_SquareRoot0(Mips_CVecDotProd(s1, s1)) << 6;
					TransposeMatrix(&mOldRotMatrix, &m2);

					ang.vz = 0;
					ang.vx = 0;
					ang.vy = (a - b) & 4095;
					RotMatrixYXZ(&ang, &m);
					MulMatrix(&m2, &m);

					m3 = mOldRotMatrix;
					MulMatrix(&m3, &m2);

					tmp.vx = m3.m[0][0];
					tmp.vy = m3.m[0][1];
					tmp.vz = m3.m[0][2];
					r.vx = Mips_CVecDotProd(&tmp, s1);
					tmp.vx = m3.m[1][0];
					tmp.vy = m3.m[1][1];
					tmp.vz = m3.m[1][2];
					r.vy = Mips_CVecDotProd(&tmp, s1);
					tmp.vx = m3.m[2][0];
					tmp.vy = m3.m[2][1];
					tmp.vz = m3.m[2][2];
					r.vz = Mips_CVecDotProd(&tmp, s1);
					*s1 = r;
					l1 = M3dMaths_SquareRoot0(Mips_CVecDotProd(s1, s1)) << 6;
					l0 >>= 8;
					l1 >>= 8;
					*s1 = (*s1 * l0) / l1;
				}
			}
		}
	}
}

/* ReflectHeadingVelocityInPlane__6CBruceG7SVECTORi (0x80039f2c) */
void CBruce::ReflectHeadingVelocityInPlane(SVECTOR Normal, int Angle) {
	CVector TmpCV;
	int dot;
	MATRIX SurfaceRotMatrix;
	CVector OldVel;

	if (!Angle)
		Angle = 25;

	OldVel = mVel;

	{
		int _x = Normal.vx;
		int _y = Normal.vy;
		int _z = Normal.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd(&mVel, &TmpCV);

	if (dot < 0) {
		mVel.vx -= Mips_FixMul(Normal.vx, dot);
		mVel.vy -= Mips_FixMul(Normal.vy, dot);
		mVel.vz -= Mips_FixMul(Normal.vz, dot);
		mVel.vx += Mips_FixMul(Normal.vx, 205);
		mVel.vy += Mips_FixMul(Normal.vy, 205);
		mVel.vz += Mips_FixMul(Normal.vz, 205);
	}

	{
		int _x = -Normal.vx;
		int _y = -Normal.vy;
		int _z = -Normal.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd(&mFront, &TmpCV);

	if (dot < 0) {
		int sin;
		int cos;
		short *pShort;
		SVECTOR RotAngle;

		sin = M3dMaths_SquareRoot0(0x1000000 - dot * dot);
		pShort = (short *)&SurfaceRotMatrix;
		cos = dot;
		dot = Mips_CVecDotProd(&mRight, &TmpCV);

		if (dot > 0)
			sin = -sin;

		pShort[0] = cos;
		pShort[1] = 0;
		pShort[2] = -sin;
		pShort[3] = 0;
		pShort[4] = 4096;
		pShort[5] = 0;
		pShort[6] = sin;
		pShort[7] = 0;
		pShort[8] = cos;
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);

		RotAngle.vz = 0;
		RotAngle.vx = 0;
		RotAngle.vy = (dot > 0 ? Angle - 1024 : 1024 - Angle) & 4095;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
		FRUVectorsFromRotMatrix();
		M3dUtils_NormaliseVector(&mFront);
		M3dUtils_NormaliseVector(&mUp);
		geom_cross(&mFront, &mUp, &mRight, 12);
		M3dUtils_NormaliseVector(&mRight);
		geom_cross(&mUp, &mRight, &mFront, 12);
		RotMatrixFromFRUVectors();
	}
}

/* AlignToFallLine__6CBrucei (0x8003a214) */
/* Fuzzy demo twin (0.72; retail is a trimmed version), locals from its
   MAIN.SYM. When the skater is moving across a slope, turns them towards the fall line: the turn
   rate grows with the slope's steepness and shrinks with speed. It's held at 0 while the skater
   faces within a dead zone either side of the fall line's perpendicular. */
void CBruce::AlignToFallLine(int Amount) {
	int Speed;
	int dot;
	int PolyTilt;
	int angle;
	CVector FallLinePerp;
	CVector FallLineUnit;
	VECTOR mLPN;
	CVector FrontInPlane;

	mFallLineTurn = 0;

	if (mInAir)
		return;

	Speed = SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;
	PolyTilt = SquareRoot0(Mips_CVecDotProd(&mFallLine, &mFallLine)) << 6;
	angle = PolyTilt / 260;
	angle = angle * PolyTilt / 13000;

	if (mMaxSpeed < Speed)
		Speed = mMaxSpeed;

	angle = angle * ((Amount << 12) + mMaxSpeed - Speed) / mMaxSpeed;

	if (!angle)
		return;

	if (mFallLineFrame + 10 < XFrames)
		mFallLineSide = 0;

	mFallLineFrame = XFrames;
	FallLineUnit = mFallLine;
	M3dUtils_NormaliseVector(&FallLineUnit);

	mLPN.vx = mPolyNormal.vx;
	mLPN.vy = mPolyNormal.vy;
	mLPN.vz = mPolyNormal.vz;
	geom_cross(&mLPN, &FallLineUnit, &FallLinePerp, 12);

	FrontInPlane = mFront;
	ProjectCVectorToPlane(&FrontInPlane, mPolyNormal);

	dot = Mips_CVecDotProd(&FrontInPlane, &FallLineUnit);
	dot = Mips_CVecDotProd(&mFront, &FallLinePerp);

	if (dot >= 42) {
		if (mFallLineSide != 2) {
			mFallLineTurn = -angle;
			mFallLineSide = 1;
		}
	} else if (dot < -41) {
		if (mFallLineSide != 1) {
			mFallLineTurn = angle;
			mFallLineSide = 2;
		}
	} else {
		mFallLineSide = 0;
	}
}

/* UprightSideways__6CBruce (0x8003a498) */
/* Demo twin; locals from its MAIN.SYM. Rolls the skater a notch back toward
   upright: the front x world-up cross product (GTE op) dotted with the up vector picks a roll
   direction, applied by multiplying in a Z rotation. */
void CBruce::UprightSideways() {
	CVector TmpCV;
	int dot;
	MATRIX SurfaceRotMatrix;
	SVECTOR RotAngle;
	VECTOR v;

	geom_cross(&mFront, &WorldUp, &v, 12);

	{
		int _x = v.vx;
		int _y = v.vy;
		int _z = v.vz;

		TmpCV.vx = _x;
		TmpCV.vy = _y;
		TmpCV.vz = _z;
	}

	dot = Mips_CVecDotProd(&mUp, &TmpCV);

	if (dot >= 42) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 11;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
	} else if (dot < -41) {
		RotAngle.vy = 0;
		RotAngle.vx = 0;
		RotAngle.vz = 4085;
		RotMatrixYXZ(&RotAngle, &SurfaceRotMatrix);
		MulMatrix(&mRotMatrix, &SurfaceRotMatrix);
	} else {
		Mips_CVecDotProd(&mUp, &WorldUp);
	}

	FRUVectorsFromRotMatrix();
}

/* FRUVectorsFromRotMatrix__6CBruce (0x8003a5a4) */
/* Front, right and up vectors from the rotation matrix's columns 2, 0 and 1. */
void CBruce::FRUVectorsFromRotMatrix() {
	mFront.vx = mRotMatrix.m[0][2];
	mFront.vy = mRotMatrix.m[1][2];
	mFront.vz = mRotMatrix.m[2][2];
	mRight.vx = mRotMatrix.m[0][0];
	mRight.vy = mRotMatrix.m[1][0];
	mRight.vz = mRotMatrix.m[2][0];
	mUp.vx = mRotMatrix.m[0][1];
	mUp.vy = mRotMatrix.m[1][1];
	mUp.vz = mRotMatrix.m[2][1];
}

/* RotMatrixFromFRUVectors__6CBruce (0x8003a5fc) */
/* The rotation matrix's columns 0, 1 and 2 from the right, up and front vectors. */
void CBruce::RotMatrixFromFRUVectors() {
	mRotMatrix.m[0][0] = mRight.vx;
	mRotMatrix.m[1][0] = mRight.vy;
	mRotMatrix.m[2][0] = mRight.vz;
	mRotMatrix.m[0][1] = mUp.vx;
	mRotMatrix.m[1][1] = mUp.vy;
	mRotMatrix.m[2][1] = mUp.vz;
	mRotMatrix.m[0][2] = mFront.vx;
	mRotMatrix.m[1][2] = mFront.vy;
	mRotMatrix.m[2][2] = mFront.vz;
}

/* SetAnglesFromHeading__6CBruce (0x8003a648) */
/* Euler angles from the rotation matrix, with its up column negated for the
   conversion and restored afterwards. */
void CBruce::SetAnglesFromHeading() {
	SVECTOR Angles;

	mRotMatrix.m[0][1] = -mRotMatrix.m[0][1];
	mRotMatrix.m[1][1] = -mRotMatrix.m[1][1];
	mRotMatrix.m[2][1] = -mRotMatrix.m[2][1];
	M3dUtils_AnglesFromMatrix(&Angles, &mRotMatrix);

	mRotMatrix.m[0][1] = -mRotMatrix.m[0][1];
	mRotMatrix.m[1][1] = -mRotMatrix.m[1][1];
	mRotMatrix.m[2][1] = -mRotMatrix.m[2][1];
	mAngles.vx = Angles.vx;
	mAngles.vy = Angles.vy;
	mAngles.vz = Angles.vz;
}

/* HandleContinuousSounds__6CBruce (0x8003a6d4) */
/* Demo twin; locals from its MAIN.SYM. Per-frame looping skater sounds:
   the state-5 loop, the state-4 (grind) loop picked by surface, the rolling loop picked by the
   ground terrain (with per-terrain volume/pitch scales), the in-air wind loop (states 1, 2, 8)
   and the speed-driven pitch/volume of the rolling loop. GetStat is the shared stat macro (see
   dofootplant.cpp). */
void CBruce::HandleContinuousSounds() {
	if (mPhysicsState == 5) {
		if (!mLoopSnd)
			mLoopSnd = SFX_PlayX(SFX_WALLRIDE_LOOP, 0x2000, 0);
	} else if (mLoopSnd) {
		SFX_Stop(mLoopSnd);

		mLoopSnd = 0;
	}

	if (mPhysicsState == 4) {
		if (!mGrindSnd) {
			ESfx Snd;

			if (m2EC && (m210 == 0 || m210 == 2))
				Snd = SFX_GRIND_LOOP;
			else if (!m2EC && (m210 == 0 || m210 == 2))
				Snd = SFX_SLIDE_LOOP;
			else if (GLevel == 8)
				Snd = SFX_ROSWELL_GRIND_LOOP;
			else
				Snd = SFX_GRIND_LOOP_RAIL1;

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
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN2, 0x2000, 0);
					mRollVolScale = 100;
					mRollPitchScale = 100;
					break;
				case 5:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN5, 0x2000, 0);
					break;
				case 3:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN3, 0x2000, 0);
					break;
				case 13:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN13, 0x2000, 0);
					mRollVolScale = 40;
					mRollPitchScale = 100;
					break;
				case 4:
					if (GLevel == 7) {
						mRollSnd = SFX_PlayX(SFX_STREETS_ROLL_TERRAIN4, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 110;
					} else {
						mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN13, 0x2000, 0);
						mRollVolScale = 40;
						mRollPitchScale = 90;
					}

					break;
				case 6:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN6, 0x2000, 0);
					break;
				case 8:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN8, 0x2000, 0);
					mRollVolScale = 50;
					mRollPitchScale = 110;
					break;
				case 11:
					mRollSnd = SFX_PlayX(SFX_ROLL, 0x2000, 0);
					mRollVolScale = 90;
					mRollPitchScale = 70;
					break;
				case 1:
					if (GLevel == 8) {
						mRollSnd = SFX_PlayX(SFX_ROSWELL_ROLL_TERRAIN1, 0x2000, 0);
						mRollVolScale = 150;
						mRollPitchScale = 100;
					} else
						mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN1, 0x2000, 0);

					break;
				case 12:
					if (GLevel == 8) {
						mRollSnd = SFX_PlayX(SFX_ROSWELL_ROLL_TERRAIN12, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 100;
					} else {
						mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN12, 0x2000, 0);
						mRollVolScale = 50;
						mRollPitchScale = 100;
					}

					break;
				case 10:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN10, 0x2000, 0);
					mRollVolScale = 75;
					mRollPitchScale = 150;
					break;
				case 14:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN14, 0x2000, 0);
					break;
				case 15:
					mRollSnd = SFX_PlayX(SFX_ROLL_TERRAIN15, 0x2000, 0);
					mRollVolScale = 60;
					mRollPitchScale = 150;
					break;
				default:
					mRollSnd = SFX_PlayX(SFX_ROLL, 0x2000, 0);
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
			mAirSnd = SFX_PlayX(SFX_AIR_LOOP, NewVol * 2 / 3, NewVol * 2 / 3);
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

/* OrientToNormal__6CBruce (0x8003b0c0) */
void CBruce::OrientToNormal() {
	VECTOR z1;
	CVector normal;
	CVector x1;

	if (mSameNormal < 25) {
		if (mSameNormal == 24) {
			int _x = mNormal.vx;
			int _y = mNormal.vy;
			int _z = mNormal.vz;

			normal.vx = _x;
			normal.vy = _y;
			normal.vz = _z;
		} else {
			normal.vx = mOldNormal.vx + ((mNormal.vx - mOldNormal.vx) >> 2);
			normal.vy = mOldNormal.vy + ((mNormal.vy - mOldNormal.vy) >> 2);
			normal.vz = mOldNormal.vz + ((mNormal.vz - mOldNormal.vz) >> 2);
		}

		M3dUtils_NormaliseVector(&normal);
		mFront.Set(mRotMatrix.m[0][2], mRotMatrix.m[1][2], mRotMatrix.m[2][2]);
		geom_cross(&mFront, &normal, &x1, 12);
		M3dUtils_NormaliseVector(&x1);
		geom_cross(&normal, &x1, &z1, 12);

		mRotMatrix.m[0][0] = x1.vx;
		mRight.vx = mRotMatrix.m[0][0];
		mRotMatrix.m[1][0] = x1.vy;
		mRight.vy = mRotMatrix.m[1][0];
		mRotMatrix.m[2][0] = x1.vz;
		mRight.vz = mRotMatrix.m[2][0];
		mRotMatrix.m[0][1] = normal.vx;
		mUp.vx = mRotMatrix.m[0][1];
		mRotMatrix.m[1][1] = normal.vy;
		mUp.vy = mRotMatrix.m[1][1];
		mRotMatrix.m[2][1] = normal.vz;
		mUp.vz = mRotMatrix.m[2][1];
		mRotMatrix.m[0][2] = z1.vx;
		mFront.vx = mRotMatrix.m[0][2];
		mRotMatrix.m[1][2] = z1.vy;
		mFront.vy = mRotMatrix.m[1][2];
		mRotMatrix.m[2][2] = z1.vz;
		mFront.vz = mRotMatrix.m[2][2];

		{
			int _x = normal.vx;
			int _y = normal.vy;
			int _z = normal.vz;

			mOldNormal.vx = _x;
			mOldNormal.vy = _y;
			mOldNormal.vz = _z;
		}
	}
}

/* HandlePlayerCollisions__6CBruce (0x8003b2f0) */
/* Two-player skater-vs-skater collision: when both skaters are on the ground and
   within 60 units, compares each one's velocity along the line between them; the one moving into
   the other (or the faster of two closing skaters) knocks the other down, else bails itself.
   The other skater pointer lives at 0x1d4 (bruce.h has only the pad there). */
void CBruce::HandlePlayerCollisions() {
	int s0, s1, v;
	CVector *s3;

	/* (with no other skater retail read one from low RAM) */
	if (mInAir == 0 && mpOtherSkater) {
		if (mpOtherSkater->mInAir == 0) {
			CVector d = mpOtherSkater->mPos - mPos;

			if (d.Length() < 60) {
				CVector c = mpOtherSkater->mPos - mOldPos;

				d = c;
				d.Normalize();

				s3 = &mVel;
				s1 = Mips_CVecDotProd(s3, &d);
				v = Mips_CVecDotProd(&mpOtherSkater->mVel, &d);

				if (s1 > 0) {
					if (v > 0) {
						mpOtherSkater->Bail(0);
					} else {
						s0 = mpOtherSkater->mVel.Length();

						if (s3->Length() >= s0)
							mpOtherSkater->Bail(0);
						else
							Bail(0);
					}
				} else if (v <= 0)
					Bail(0);
			}
		}
	}
}

/* DoPhysics__6CBruce (0x8003b424) */
/* THPS1's per-frame skater update. The THPS2 demo split most of this into
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
			Group[9] = DAT_800a727c;
			Group[10] = DAT_800a72a8;
			Group[6] = DAT_800a7250;
			break;
		case 1:
			Group[9] = DAT_800a7300;
			Group[10] = DAT_800a732c;
			Group[6] = DAT_800a72d4;
			break;
		case 2:
			Group[9] = DAT_800a7250;
			Group[10] = DAT_800a72a8;
			Group[6] = DAT_800a72d4;
			break;
		case 3:
			Group[9] = DAT_800a727c;
			Group[10] = DAT_800a732c;
			Group[6] = DAT_800a7250;
			break;
		case 4:
			Group[9] = DAT_800a727c;
			Group[10] = DAT_800a732c;
			Group[6] = DAT_800a7250;
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
			SetGlow(0, 0, 0, 5);
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
					SFX_PlayX(SFX_COMP_TIME_UP, 0x2000, 0);

				Game_Over();
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

			if (BrakeVal < M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6) {
				if (mAnim != 49) {
					mAnimSpeed = 0x20000;
					SFX_PlayX(SFX_SKID, 0x2000, 0);
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
		int NewMag = (M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6) >> 12;

		mAcc.vx += mVel.vx / NewMag;
		mAcc.vy += mVel.vy / NewMag;
		mAcc.vz += mVel.vz / NewMag;
		m33C--;
	}

	if (mMaxSpeed < M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6)
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
				SFX_PlayX(SFX_GAP, 0x2000, 0);

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
					SFX_PlayX(SFX_IMPACT_1, 0x2000, 0);
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
					SFX_PlayX(SFX_IMPACT_1, 0x2000, 0);
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

	HandlePlayerCollisions();

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
		n = this == GSkater[0] ? 1 : 2;
		Trig_ResetCPCollisionFlags(n);

		pCurrentPlayer = this;
		M3dZone_LineToItemNotSCached(&LInfo, 1);
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
	Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;

	if (Spd > MAXSPEED(50)) {
		mVel.vx = mVel.vx * (MAXSPEED(50) >> 12) / (Spd >> 12);
		mVel.vy = mVel.vy * (MAXSPEED(50) >> 12) / (Spd >> 12);
		mVel.vz = mVel.vz * (MAXSPEED(50) >> 12) / (Spd >> 12);
		Spd = M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;
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

/* FUN_8003d6bc (0x8003d6bc) */
/* Resets the gravity vector (DAT_800a7f28) to straight down. */
void FUN_8003d6bc() {
	DAT_800a7f28.vx = 0;
	DAT_800a7f28.vy = 0x8000;
	DAT_800a7f28.vz = 0;
}
