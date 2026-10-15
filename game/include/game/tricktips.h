/* The trick-tip monitor (TRICKTIPS.CPP). */
#ifndef GAME_TRICKTIPS_H
#define GAME_TRICKTIPS_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/class.h"

class CBruce;
#include "game/mess.h"

/* the trick-tip monitor (one global, TrickTipMonitor; layout = demo, vtable 0x800a2498) */
class CTrickTipMonitor : public CClass {
  public:
	/* 0x000: vptr */
	int mShowAsNeededQueue[6];     /* 0x004 */
	int mShowAsNeededIndex;        /* 0x01c */
	int mShowAfterBailQueue[6];    /* 0x020 */
	int mShowAfterBailIndex;       /* 0x038 */
	int mShowAtStartQueue[6];      /* 0x03c */
	int mShowAtStartIndex;         /* 0x054 */
	uint mTimer;                   /* 0x058 */
	uint mLastTipTime;             /* 0x05c */
	uint mJumpTimer;               /* 0x060 */
	CBruce *pSkater;               /* 0x064 */
	char mTipLine1Buf[40];         /* 0x068 */
	char *mTipLine1;               /* 0x090 */
	char *mTipLine2;               /* 0x094 */
	char *mTipLine3;               /* 0x098 */
	char *mTipLine4;               /* 0x09c */
	uint mTipTimer;                /* 0x0a0 */
	int mOldBails;                 /* 0x0a4 */
	int mOldPerpendicularBails;    /* 0x0a8 */
	int mOldTrickBails;            /* 0x0ac */
	int mOldRailBails;             /* 0x0b0 */
	int mOldJumps;                 /* 0x0b4 */
	int mOldWeakJumps;             /* 0x0b8 */
	int mOldAirs;                  /* 0x0bc */
	int mOldTricks;                /* 0x0c0 */
	int mOldRails;                 /* 0x0c4 */
	int mOldAirPolys;              /* 0x0c8 */
	int mOldAirPolyBreaks;         /* 0x0cc */
	int mOldBigAirs;               /* 0x0d0 */
	int mOldGrinds;                /* 0x0d4 */
	int mBailCounter;              /* 0x0d8 */
	int mPerpendicularBailCounter; /* 0x0dc */
	int mTrickBailCounter;         /* 0x0e0 */
	int mRailBailCounter;          /* 0x0e4 */
	int mJumpCounter;              /* 0x0e8 */
	int mWeakJumpCounter;          /* 0x0ec */
	int mAirCounter;               /* 0x0f0 */
	int mTrickCounter;             /* 0x0f4 */
	int mRailCounter;              /* 0x0f8 */
	int mAirPolyCounter;           /* 0x0fc */
	int mAirPolyBreakCounter;      /* 0x100 */
	int mBigAirCounter;            /* 0x104 */
	int mGrindCounter;             /* 0x108 */
	int mHoldingForwardCounter;    /* 0x10c */
	int mOkayToShow;               /* 0x110 (TrickTips_Init sets 1) */
	/* sizeof(CTrickTipMonitor) == 0x114 */

	/* inlined into TrickTips_Update (tricktips/update.cpp defines them `inline`) */
	int FindTip();
	void ShowTip(int tip);
	void ShowFoundTip(int tip);
	void Update();
};

/* functions */
void TrickTips_Display();
void TrickTips_Init();
void TrickTips_Update();

#endif
