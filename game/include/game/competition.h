/* CCompetition / CJudge (COMPETITION.H / COMPETITION.CPP): the competition state, a single global
   (theCompetition, 0x800c0fdc) built by the module's static constructor (_GLOBAL_.I 0x800770dc)
   and torn down by _GLOBAL_.D (0x80077060). Both classes are CClass objects (vptr at +0): CCompetition
   vtable 0x800a3444 (slot 0 = its implicit destructor 0x80077130), CJudge vtable 0x800a3454
   (slot 0 = its implicit destructor 0x800771c4). Neither declares a destructor: retail's are the
   compiler-synthesized ones (they never re-store their own vptr). Layout = the demo's.
   See build/hdr_notes/competition.md. */
#ifndef GAME_COMPETITION_H
#define GAME_COMPETITION_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/class.h"

class CPowerUp;

/* the lines shown when a competition medal unlocks something (MedalMessage[3]) */
struct SMedalMessage {
	char *Line1; /* 0x0 */
	char *Line2; /* 0x4 */
	char *Line3; /* 0x8 */
};

class CJudge : public CClass {
  public:
	uchar mProPlayer; /* 0x4 */
	uchar mProBias;   /* 0x5 */
	uchar mConPlayer; /* 0x6 */
	uchar mConBias;   /* 0x7 */
	                  /* sizeof(CJudge) == 0x8 */
};

class CCompetition : public CClass {
  public:
	uchar mCompetitorList[10]; /* 0x004 */
	char pad0e[0x10 - 0xe];
	uint mJudgeTimer;       /* 0x010 */
	uint mLadderTimer;      /* 0x014 */
	int mWinLoseCheer;      /* 0x018 */
	uchar mLastLadderEntry; /* 0x01c */
	char pad1d[0x20 - 0x1d];
	int mJudgeFullyDisplayed;  /* 0x020 */
	int mLadderFullyDisplayed; /* 0x024 */
	CJudge mJudges[5];         /* 0x028 */
	short mJudgesScore[5];     /* 0x050 */
	uchar mHighScore;          /* 0x05a */
	uchar mLowScore;           /* 0x05b */
	uchar mPlacing[10];        /* 0x05c */
	uchar mOrdering[10];       /* 0x066 */
	ushort mScore[10][3];      /* 0x070 */
	ushort mTotalScore[10];    /* 0x0ac */
	uchar mCurrentHeat;        /* 0x0c0 */
	char padc1[0xc4 - 0xc1];
	int medal_mask;           /* 0x0c4 */
	CPowerUp *mMedal1;        /* 0x0c8 */
	CPowerUp *mMedal2;        /* 0x0cc */
	CPowerUp *mMedal3;        /* 0x0d0 */
	char *mMarqueeText;       /* 0x0d4 */
	char mMarqueeTextBuf[75]; /* 0x0d8 */
	char pad123[0x124 - 0x123];
	uint mMarqueeTimer; /* 0x124 */
	int our_skater_num; /* 0x128 */
	int mFading;        /* 0x12c */
	/* sizeof(CCompetition) == 0x130 */

	/* inlined into _GLOBAL_.I (0x800770dc) */
	CCompetition() {
		mMedal1 = 0;
		mMedal2 = 0;
		mMedal3 = 0;
	}
};

extern CCompetition theCompetition;

/* functions */
int Competition_WonGameAllGold();      /* 0x8007401c */
void Competition_FullyDisplayJudge();  /* 0x80076484 */
void Competition_FullyDisplayLadder(); /* 0x80076494 */

#endif
