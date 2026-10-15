/* The on-screen panel and trick scoring (PANEL.CPP), the trick tables (PHYSICS) and the trick-tip
   monitor (TRICKTIPS.CPP).

   CPanelData is embedded by value in CBruce at +0x924. Retail's is 0x4f8 bytes (the demo's
   0x8e4): 0x000..0x163 are the demo's; retail drops the three Fonts and the TrickTextEffect,
   keeps two (non-virtual, 0x48-byte) FallingTextEffects, and shrinks CTrick to 20 bytes.

   Local stand-ins: STrickInfo (physics/doinair, physics/dophysics: `int w[11]` / `f18`) is STrick;
   STrickScore (physics/dophysics: the m2E0 pointee) is SGapTrick. */
#ifndef GAME_PANEL_H
#define GAME_PANEL_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

/* SDK headers wrapped as in m3d.h (Panel_DrawFlatShadedPoly returns a POLY_F4) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"
#include "game/class.h"
#include "game/mess.h"
struct SAnimFrame;
class Sprite;

class CBruce;
struct Message;

/* one queued trick on the panel (demo: 0x24 bytes with spins/locked/count/switched) */
class CTrick {
  public:
	char *name;       /* 0x00 */
	int point_value;  /* 0x04 */
	int depreciate;   /* 0x08 */
	int countdown;    /* 0x0c */
	Message *message; /* 0x10 */
	/* sizeof(CTrick) == 0x14 */

	void Set(Message *pMessage, char *pName, int Points, int Depreciate);
};

/* the trick table entry (`STrick Group[]`, 44 bytes; entries from 32 on are rail tricks keyed by
   style + two button codes) */
struct STrick {
	int Flags;    /* 0x00 */
	int Anim;     /* 0x04 */
	int w2;       /* 0x08 */
	char *pName;  /* 0x0c */
	int Frames;   /* 0x10 */
	int w5;       /* 0x14 */
	int MinFrame; /* 0x18 */
	int Score;    /* 0x1c */
	int w8;       /* 0x20 */
	int w9;       /* 0x24 */
	int w10;      /* 0x28 */
	              /* sizeof(STrick) == 0x2c */
};

/* a gap bonus (TRIG; CBruce::mpGrindGapTrick points at one) */
struct SGapTrick {
	short Type;  /* 0x0 */
	short Score; /* 0x2 */
	char *Name;  /* 0x4 */
};

/* one trick tip (`STip TRICKTIP[]`) */
struct STip {
	int Flag;    /* 0x00 */
	int m4;      /* 0x04 */
	char *Line1; /* 0x08 */
	char *Line2; /* 0x0c */
	char *Line3; /* 0x10 */
};

class CPanelData {
  public:
	CBruce *mpBruce;                      /* 0x000 */
	uint mTotalScore;                     /* 0x004 */
	ushort mPlayerNum;                    /* 0x008 */
	ushort mNumTricksLanded;              /* 0x00a */
	ushort mNumBails;                     /* 0x00c */
	ushort mNumTricksInCombo;             /* 0x00e */
	uint mBestComboScore;                 /* 0x010 */
	char mBestComboName[100];             /* 0x014 */
	char mBestComboName2[100];            /* 0x078 */
	char mBestComboName3[100];            /* 0x0dc */
	uint mLandedTotal;                    /* 0x140 */
	uint mLandedScore;                    /* 0x144 */
	uint mLandedMultiplier;               /* 0x148 */
	int mTrickState;                      /* 0x14c (demo: mFallingEnabled) */
	int mTrickTimer;                      /* 0x150 (demo: mWantToFall) */
	int mLastHoldingScore;                /* 0x154 */
	int mLastMult;                        /* 0x158 */
	int mBailFlag;                        /* 0x15c (demo name; unproven) */
	int mOutOfBounds;                     /* 0x160 (demo name; unproven) */
	FallingTextEffect mFallingTextEffect; /* 0x164 */
	int mOutOfBoundsMessage;              /* 0x1ac: 1..4, which out-of-bounds message */
	FallingTextEffect mOutOfBoundsText;   /* 0x1b0 */
	CTrick mTQ[20];                       /* 0x1f8: the trick queue */
	int mTQStack;                         /* 0x388: tricks in the queue */
	char mTrickName[100];                 /* 0x38c */
	char mTrickName2[100];                /* 0x3f0 */
	char mTrickName3[100];                /* 0x454 */
	int old_180spins;                     /* 0x4b8 (demo name; unproven) */
	int m4BC;                             /* 0x4bc */
	int mTrickStatus;                     /* 0x4c0 */
	int m4C4;                             /* 0x4c4 */
	int SKATE[5];                         /* 0x4c8: the S-K-A-T-E letters collected */
	int Goals;                            /* 0x4dc */
	int m4E0;                             /* 0x4e0 */
	int m4E4;                             /* 0x4e4 */
	int m4E8;                             /* 0x4e8 */
	int m4EC;                             /* 0x4ec */
	int m4F0;                             /* 0x4f0 */
	int m4F4; /* 0x4f4: bail timer (Land sets 40; displayscore: `Bailing`) */
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
	void DisplayTrick(CBruce *pBruce, ushort xpos, ushort ypos);
	void TrickMessageRefresh();
	void TrickMessage();
	void Trick(char *pName, int Points, int Dep);
	void Land();
	void OutOfBounds();
	int TotalScore();
};

/* functions */
void Panel_addPrim(void *p);
void FUN_8006c4c4();
void Panel_Update();
POLY_FT4 *Panel_DrawTexturedPoly(SAnimFrame *t);
void Panel_SetStretchedScreenCoords(int x, int y, POLY_FT4 *p, SAnimFrame *f);
POLY_F4 *Panel_DrawFlatShadedPoly(int x, int y, int w, int h, uchar r, uchar g, uchar b, int sort);
void GraffitiColor(struct CBruce *a);
void Panel_PlayerInit(unsigned short n, CBruce *b);
void Panel_TakeOff(CBruce *p, int a);
void Panel_Trick(CBruce *p, char *name, int score, int a3);
void Panel_HoldTrick(CBruce *p, int s);
void Panel_Bonus(CBruce *pSkater, int Score);
void Panel_Land(CBruce *p);
void Panel_Bail(CBruce *p);
int Panel_Letter(int n);
void Panel_Tape(int a);
void Panel_GoalCounter(int goals);
char *Panel_SwitchText(CBruce *p);

/* globals */
extern int GStartTime;
extern Sprite *pBarSprite;
extern Sprite *pScoreSprite;
#endif
