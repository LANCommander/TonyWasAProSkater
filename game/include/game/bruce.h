/* The skater, CBruce (Neversoft codename "Bruce"; MECH.CPP + PHYSICS.CPP), derived from CSuper.

   Retail's CBruce is 0xe1c bytes and laid out very differently from the demo's (0x146c, with
   CPanelData at +0x190): retail puts its own data at 0x198..0x923 and the 0x4f8-byte CPanelData
   at +0x924 (CBruce::CBruce inlines CPanelData's ctor; ~CBruce destroys the two
   FallingTextEffects at +0xa88/+0xad4). Offsets/sizes come from the byte-exact TUs (class census);
   names are the TUs' (or the demo's where a run of fields lines up), m<offset> otherwise.

   Not real classes: `CSkater` (main/gamelogic, main/playaway) and `Tripod` (camera/loadintomike)
   are local stand-ins for CBruce. See build/hdr_notes/bruce.md. */
#ifndef GAME_BRUCE_H
#define GAME_BRUCE_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/classsizes.h"
#include "game/types.h"
#include "game/vector.h"
#include "game/mem.h"
#include "game/ob.h"
#include "game/pad.h"
#include "game/panel.h"
#include "game/rail.h"

class CCar;
class CCamera;
struct Item;

enum EPhysicsState {
	PHYSICS_ON_GROUND,
	PHYSICS_IN_AIR,
	PHYSICS_ON_INVISIBLE,
	PHYSICS_IN_AIR_STICK_TO,
	PHYSICS_ON_RAIL,
	PHYSICS_IN_WALLRIDE,
	PHYSICS_IN_FOOTPLANT,
	PHYSICS_STOPPED,
	PHYSICS_IN_HANDPLANT,
	PHYSICS_MANUAL
};

enum ELimitBailVel { DO_LIMIT, DONT_LIMIT };

class CBruce : public CSuper {
  public:
	uchar *mpStatBonus;    /* 0x198: per-stat board bonus */
	int mBailNumber;       /* 0x19c */
	CCar *mLastCar;        /* 0x1a0: car last collided with */
	CVector m1A4;          /* 0x1a4 */
	CVector m1B0;          /* 0x1b0 */
	CVector mSafePos;      /* 0x1bc */
	CVector mGravity;      /* 0x1c8 (ctor: 0,1000,0) */
	CBruce *mpOtherSkater; /* 0x1d4 the other player's skater (HandlePlayerCollisions) */
	CCamera *mpCamera;     /* 0x1d8 */
	CVector m1DC;          /* 0x1dc */
	int m1E8;              /* 0x1e8 */
	int m1EC;              /* 0x1ec */
	int mGraffitiCount;    /* 0x1f0 */
	int mGraffitiSteals;   /* 0x1f4 */
	int m1F8;    /* 0x1f8 */
	int m1FC;    /* 0x1fc */
	int m200;    /* 0x200 */
	int m204;    /* 0x204 */
	int m208;    /* 0x208 */
	int mGrinds; /* 0x20c */
	ERailType mRailType; /* 0x210 (the rail being ground's CRail::mType) */
	int m214;    /* 0x214 */
	/* 0x218..0x247: the trick-tip counters (demo order) */
	int mBails;              /* 0x218 */
	int mPerpendicularBails; /* 0x21c */
	int mTrickBails;         /* 0x220 */
	int mRailBails;          /* 0x224 */
	int mJumps;              /* 0x228 */
	int mWeakJumps;          /* 0x22c */
	int mAirs;               /* 0x230 */
	int mTricks;             /* 0x234 */
	int mRails;              /* 0x238 */
	int mAirPolys;           /* 0x23c */
	int mAirPolyBreaks;      /* 0x240 */
	int mBigAirs;            /* 0x244 */
	int m248;                /* 0x248 */
	int m24C;                /* 0x24c */
	int mBalanceTime;        /* 0x250 (panel/displayscore `BalanceTime`) */
	int mBalanceFlash;       /* 0x254 (panel/displayscore `BalanceFlash`; GetStat: +3 while set) */
	int mFirstLeftRightPressed; /* 0x258 */
	int m25C;                   /* 0x25c */
	int m260;                   /* 0x260 */
	int m264;                   /* 0x264 */
	int m268;                   /* 0x268 */
	int m26C;                   /* 0x26c */
	int m270;                   /* 0x270 */
	int m274;                   /* 0x274 */
	int m278;                   /* 0x278 */
	int m27C;                   /* 0x27c */
	int m280;                   /* 0x280 */
	int mNoSpin;                /* 0x284 */
	int mNoSpinScore;           /* 0x288 */
	int mGapChecksum[10];       /* 0x28c */
	int mGapType[10];           /* 0x2b4 */
	int mGaps;                  /* 0x2dc */
	SGapTrick *mpGrindGapTrick; /* 0x2e0 */
	int m2E4;                   /* 0x2e4 */
	int mSkaterIndex;           /* 0x2e8 */
	int m2EC;                   /* 0x2ec */
	int mFlipPending;           /* 0x2f0: toggle mExtraFlags bit 1 at the next anim change */
	int mFlipAxesPending;       /* 0x2f4: mirror the front/right axes at the next anim change */
	int m2F8;                   /* 0x2f8 */
	int m2FC;                   /* 0x2fc */
	int m300;                   /* 0x300 */
	int m304;                   /* 0x304 */
	int mFallLineSide;          /* 0x308 */
	int mFallLineFrame;         /* 0x30c */
	int m310;                   /* 0x310 */
	int m314;                   /* 0x314 */
	int mDoingSharpTurn;        /* 0x318 */
	int mTurning;               /* 0x31c */
	int m320;                   /* 0x320 */
	int mTapTurn;               /* 0x324 */
	int mTallyAngles;           /* 0x328 */
	int mNum180Spins;           /* 0x32c */
	int mTime;                  /* 0x330: frames left in the run */
	int mMaxSpeed;              /* 0x334 */
	int mKickSpeed;             /* 0x338 */
	int m33C;                   /* 0x33c */
	int m340;                   /* 0x340 */
	int m344;                   /* 0x344 */
	int m348;                   /* 0x348 */
	int m34C;                   /* 0x34c */
	char pad350[0x358 - 0x350];
	int mInAir;             /* 0x358 */
	int mDoingATrick;       /* 0x35c */
	int m360;               /* 0x360 */
	int m364;               /* 0x364 */
	int m368;               /* 0x368 */
	int m36C;               /* 0x36c */
	int m370;               /* 0x370 */
	int m374;               /* 0x374 */
	int m378[10];           /* 0x378 */
	int m3A0[10];           /* 0x3a0 */
	int m3C8[10];           /* 0x3c8 */
	int m3F0[10];           /* 0x3f0 */
	Item *mpWallItem;       /* 0x418 */
	SMemHandle mWallHandle; /* 0x41c */
	int m424;               /* 0x424 */
	int m428[80];           /* 0x428: per-trick counts */
	int m568[80];           /* 0x568: per-trick counts this combo */
	int m6A8;               /* 0x6a8 */
	char pad6ac[0x6c4 - 0x6ac];
	int mLoopSnd;                   /* 0x6c4 */
	int mGrindSnd;                  /* 0x6c8 (~CBruce stops it) */
	int mRollSnd;                   /* 0x6cc (~CBruce stops it) */
	int mAirSnd;                    /* 0x6d0 */
	int mAirPitch;                  /* 0x6d4 */
	int mAirVol;                    /* 0x6d8 */
	int m6DC;                       /* 0x6dc */
	int mStickToRail;               /* 0x6e0 */
	int m6E4;                       /* 0x6e4 */
	int mRailStart;                 /* 0x6e8 */
	int mRailNode;                  /* 0x6ec */
	int m6F0;                       /* 0x6f0 */
	CVector mRailPos;               /* 0x6f4 (demo order: mRailPos, mRailDir) */
	CVector mRailDir;               /* 0x700 */
	int m70C;                       /* 0x70c */
	int m710;                       /* 0x710 (ctor: -1) */
	int m714;                       /* 0x714 */
	int m718;                       /* 0x718 */
	int m71C;                       /* 0x71c */
	int m720;                       /* 0x720 (`Flipping`) */
	int m724;                       /* 0x724 (`FlipAngle`) */
	int m728;                       /* 0x728 */
	int mTerrain;                   /* 0x72c */
	int mRollTerrain;               /* 0x730 */
	EPhysicsState mPhysicsState;    /* 0x734 */
	EPhysicsState mOldPhysicsState; /* 0x738 (demo order; ctor sets both to 1) */
	int mManIsCrouched;             /* 0x73c */
	int m740;                       /* 0x740 */
	int mJumpPending;               /* 0x744 */
	int mJumpStrength;              /* 0x748 */
	int m74C;                       /* 0x74c */
	int m750;                       /* 0x750 */
	int m754;                       /* 0x754 */
	int mRollVolScale;              /* 0x758 */
	int mRollPitchScale;            /* 0x75c */
	int m760;                       /* 0x760 */
	int m764;                       /* 0x764 */
	int m768;                       /* 0x768 */
	MATRIX mOldRotMatrix;           /* 0x76c */
	MATRIX mRotMatrix;              /* 0x78c */
	/* 0x7ac..0x83f: demo order (mTempRotationMatrix .. mAnimLeanAngle), plus m814/mPitchAngle */
	MATRIX mTempRotMatrix; /* 0x7ac */
	CVector mTempPos;      /* 0x7cc */
	CVector mFront;        /* 0x7d8 */
	CVector mRight;        /* 0x7e4 */
	CVector mUp;           /* 0x7f0 */
	CVector mFallLine;     /* 0x7fc (demo: mLastFallLine) */
	int mFallLineTurn;     /* 0x808 (demo: mFallLineAdjustment) */
	SVECTOR mPolyNormal;   /* 0x80c (demo: mLastPolyNormal) */
	CVector m814;          /* 0x814 */
	int mSameNormal;       /* 0x820 */
	VECTOR mOldNormal;     /* 0x824 */
	int mPitchAngle;       /* 0x834 */
	int mLeanAngle;        /* 0x838 */
	int mAnimLeanAngle;    /* 0x83c */
	int m840;              /* 0x840 */
	char pad844[0x868 - 0x844];
	CItem *m868;                      /* 0x868 */
	SControl *mpPad;                  /* 0x86c */
	int mPadIndex;                    /* 0x870 */
	uchar mGlowR;                     /* 0x874 */
	uchar mGlowG;                     /* 0x875 */
	uchar mGlowB;                     /* 0x876 */
	uchar mTargetGlowR;               /* 0x877 */
	uchar mTargetGlowG;               /* 0x878 */
	uchar mTargetGlowB;               /* 0x879 */
	uchar mGlowSpeed;                 /* 0x87a */
	uchar mGlowDownSpeed;             /* 0x87b */
	int mBrightness;                  /* 0x87c (demo order; ob/calcdynlight `mLight`) */
	int mAllowDeathCam;               /* 0x880 (demo order; unproven) */
	int mIgnoreBruceInputTimer;       /* 0x884 */
	int mIgnoreBruceInputFreezeTimer; /* 0x888 */
	char pad88c[0x894 - 0x88c];
	signed char mAX; /* 0x894 */
	signed char mAY; /* 0x895 */
	char pad896[0x8a4 - 0x896];
	int m8A4; /* 0x8a4 */
	int m8A8; /* 0x8a8 */
	char pad8ac[0x8cc - 0x8ac];
	CVector m8CC;          /* 0x8cc */
	CVector mLastScorePos; /* 0x8d8 */
	int mBalance;          /* 0x8e4 */
	int mBalanceVel;       /* 0x8e8 */
	int m8EC;              /* 0x8ec */
	int mRailTime;         /* 0x8f0 */
	int mGrindTrick;       /* 0x8f4 */
	int mNumTapes;         /* 0x8f8 */
	char pad8fc[0x8fe - 0x8fc];
	ushort m8FE; /* 0x8fe */
	char pad900[0x918 - 0x900];
	short m918; /* 0x918 */
	char pad91a[0x924 - 0x91a];
	CPanelData mPanel; /* 0x924 */
	/* sizeof(CBruce) == 0xe1c */

	void Trick_TakeOff();
	int Trick_RepeatScore(int Trick, int Score);
	void Trick_HoldTrick(int Score);
	void Trick_Bonus(int Score);
	void FrigDarkenObject();
	void HandleHoldTrick();
	void ScoreCurrentTrick();
	void SkateOffGround();
	int CheckSide(int a);
	void HandleLeaningBail(SVECTOR s);
	void HandlePlayerCollisions();
	CBruce();
	virtual ~CBruce();
	void SetGlow(int, int, int, int);
	int CanCollect(short);
	virtual void Die();
	virtual void AI();
	/* slot 3 (EveryFrame) is CBody's */
	virtual void DoLightingSetup(); /* empty */
	virtual int Hit(int, const CVector &, int); /* returns 1 */
	virtual void DeleteStuff(); /* empty */

	void TempRestore();
	void Trick_PlayerInit(ushort Player);
	void Trick_Trick(int Trick, char *pName, int Score, int);
	void Trick_Land();
	void Trick_Bail();
	void Vibrate(int a, int b, int c);
	void StateTransition(EPhysicsState NewState, int);
	void MaybeFlipAnimationTransition();
	void Debug_RunAnim(int Anim, int);
	void Debug_RunAnim1(int Anim, int a, int);
	void Debug_RunAnim2(int Anim, int a, int b, int);
	void Debug_RunAnim3(int Anim, int a, int b, int c, int);
	void ResetTrickBuffers();
	void Bail(int BailNumber);
	void TrickObRail();
	void HandleStickToRail();
	void HandleBailOnLanding(int);
	void HandleBailPerpendicular(SVECTOR Normal);
	void SetTrickFlags(STrick *pTrick);
	void HandleTricksNew();
	void HandleSpin();
	void AlignHeadingOnRail();
	void DoOnRailPhysics();
	void DoOnGroundPhysics();
	void DoInAirPhysics();
	void DoFootPlantPhysics();
	void DoHandPlantPhysics();
	void DoWallRidePhysics();
	void HandleJump();
	void HandleKick();
	void RotateHeadingAndVelocity(int a, int c, int b);
	void ReflectHeadingVelocityInPlane(SVECTOR Normal, int Angle);
	void AlignToFallLine(int Amount);
	void UprightSideways();
	void FRUVectorsFromRotMatrix();
	void RotMatrixFromFRUVectors();
	void SetAnglesFromHeading();
	void HandleContinuousSounds();
	void OrientToNormal();
	void DoPhysics();
	void BoardOff();
	void BoardOn();
	void GetStats(int Skater);
	void GlowUpdate();
	void CalculateDynamicLighting(uint *p_face, Item *p_item);
	void GetHookPosition(CVector &Pos, uchar Index);
	void DoShadowCheck();
	void InitItem(char *pName);
	void FlipToVelocity();
	void BloodSplat(int BodyPart, int MaxBloodBits);
	void HandleWater();

	/* inlined into HandleStickToRail (physics/handlesticktorail.cpp defines them `inline`) */
	void StartHandPlant(int RailTrick, int Flags);
	void StartGrind(int RailTrick);
	/* inline helpers retail inlined */
	int GetBalanceVel() {
		return mBalanceVel;
	}
	void SetM8A4(int a, int b) {
		m8A4 = a;
		m8A8 = b;
	}
};
CHECK_CLASS_SIZE(CBruce, SIZEOF_CBRUCE);

#endif
