/* The on-screen panel and trick scoring (PANEL.CPP), the trick tables (PHYSICS) and the trick-tip
   monitor (TRICKTIPS.CPP).

   CPanelData is embedded by value in CBruce at +0x924. Retail's is 0x4f8 bytes (the demo's
   0x8e4): 0x000..0x163 are the demo's; retail drops the three Fonts and the TrickTextEffect,
   keeps two (non-virtual, 0x48-byte) FallingTextEffects, and shrinks CTrick to 20 bytes.

   Local stand-ins: STrickInfo (physics/doinair, physics/dophysics: `int w[11]` / `f18`) is STrick;
   STrickScore (physics/dophysics: the m2E0 pointee) is SGapTrick. */
#ifndef GAME_PANEL_H
#define GAME_PANEL_H

#pragma interface /* see class.h */

#include "game/types.h"
#include "game/class.h"
#include "game/mess.h"

class CBruce;
struct Message;

/* one queued trick on the panel (demo: 0x24 bytes with spins/locked/count/switched) */
class CTrick {
  public:
	char *name;                    /* 0x00 */
	int point_value;               /* 0x04 */
	int depreciate;                /* 0x08 */
	int countdown;                 /* 0x0c */
	Message *message;              /* 0x10 */
	/* sizeof(CTrick) == 0x14 */

	void Set(Message *, char *, int, int); /* 0x8006e514 */
};

/* the trick table entry (`STrick Group[]`, 44 bytes; entries from 32 on are rail tricks keyed by
   style + two button codes) */
struct STrick {
	int Flags;                     /* 0x00 */
	int Anim;                      /* 0x04 */
	int w2;                        /* 0x08 */
	char *pName;                   /* 0x0c */
	int Frames;                    /* 0x10 */
	int w5;                        /* 0x14 */
	int MinFrame;                  /* 0x18 */
	int Score;                     /* 0x1c */
	int w8;                        /* 0x20 */
	int w9;                        /* 0x24 */
	int w10;                       /* 0x28 */
	/* sizeof(STrick) == 0x2c */
};

/* a gap bonus (TRIG; CBruce::mpGrindGapTrick points at one) */
struct SGapTrick {
	short Type;                    /* 0x0 */
	short Score;                   /* 0x2 */
	char *Name;                    /* 0x4 */
};

/* one trick tip (`STip TRICKTIP[]`) */
struct STip {
	int Flag;                      /* 0x00 */
	int m4;                        /* 0x04 */
	char *Line1;                   /* 0x08 */
	char *Line2;                   /* 0x0c */
	char *Line3;                   /* 0x10 */
};

class CPanelData {
  public:
	CBruce *mpBruce;               /* 0x000 */
	uint mTotalScore;              /* 0x004 */
	ushort mPlayerNum;             /* 0x008 */
	ushort mNumTricksLanded;       /* 0x00a */
	ushort mNumBails;              /* 0x00c */
	ushort mNumTricksInCombo;      /* 0x00e */
	uint mBestComboScore;          /* 0x010 */
	char mBestComboName[100];      /* 0x014 */
	char mBestComboName2[100];     /* 0x078 */
	char mBestComboName3[100];     /* 0x0dc */
	uint mLandedTotal;             /* 0x140 */
	uint mLandedScore;             /* 0x144 */
	uint mLandedMultiplier;        /* 0x148 */
	int mTrickState;               /* 0x14c (demo: mFallingEnabled) */
	int mTrickTimer;               /* 0x150 (demo: mWantToFall) */
	int mLastHoldingScore;         /* 0x154 */
	int mLastMult;                 /* 0x158 */
	int mBailFlag;                 /* 0x15c (demo name; unproven) */
	int mOutOfBounds;              /* 0x160 (demo name; unproven) */
	FallingTextEffect mFallingTextEffect; /* 0x164 */
	int mOutOfBoundsMessage;       /* 0x1ac: 1..4, which out-of-bounds message */
	FallingTextEffect mOutOfBoundsText;   /* 0x1b0 */
	CTrick mTQ[20];                /* 0x1f8: the trick queue */
	int mTQStack;                  /* 0x388: tricks in the queue */
	char mTrickName[100];          /* 0x38c */
	char mTrickName2[100];         /* 0x3f0 */
	char mTrickName3[100];         /* 0x454 */
	int old_180spins;              /* 0x4b8 (demo name; unproven) */
	int m4BC;                      /* 0x4bc */
	int mTrickStatus;              /* 0x4c0 */
	int m4C4;                      /* 0x4c4 */
	int SKATE[5];                  /* 0x4c8: the S-K-A-T-E letters collected */
	int Goals;                     /* 0x4dc */
	int m4E0;                      /* 0x4e0 */
	int m4E4;                      /* 0x4e4 */
	int m4E8;                      /* 0x4e8 */
	int m4EC;                      /* 0x4ec */
	int m4F0;                      /* 0x4f0 */
	int m4F4;                      /* 0x4f4: bail timer (Land sets 40; displayscore: `Bailing`) */
	/* sizeof(CPanelData) == 0x4f8 */

	/* inline in retail (CBruce::CBruce has it inlined; the demo's is out of line) */
	CPanelData(CBruce *pBruce)
	    : mpBruce(pBruce), mTotalScore(0), mLastHoldingScore(0), mLastMult(0),
	      mOutOfBoundsMessage(0) {
		mTQStack = 0;
		m4BC = 0;
		m4F4 = 0;

		for (int i = 0; i < 5; i++)
			SKATE[i] = 0;

		m4E4 = 0;
		m4E0 = 0;
		Goals = 0;
	}
	void DisplayTrick(CBruce *, ushort, ushort); /* 0x8006d938 */
	void TrickMessageRefresh();    /* 0x8006e5ac */
	void TrickMessage();           /* 0x8006e7c0 */
	void Trick(char *, int, int);  /* 0x8006e804 */
	void Land();                   /* 0x8006e8ec */
	void OutOfBounds();            /* 0x8006eba4 */
};

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
	void ShowTip(int);
	void ShowFoundTip(int);
	void Update();
};

#endif
