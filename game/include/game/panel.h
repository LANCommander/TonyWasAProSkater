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

/* A trick number: the index into the trick table Group[] (game/data/physics.cpp) for the tricks it
   holds (0-65), and for every trick its slot in the skater's repeat counts (CBruce::m428/m568),
   which CBruce::Trick_Trick bumps and Trick_RepeatScore depreciates the score by. The flips are
   Flip + d-pad, the slot being the direction bits (1 up, 2 down, 4 left, 8 right); the grabs are
   Grab + d-pad at TRICK_GRAB + direction. Slots with no trick (impossible directions, Anim 78) are
   left out. Flips 6, 9 and 10 are blank in the table and filled per skater from Stats[].Stats[4]
   by DoPhysics. The specials (from TRICK_SPECIAL) are three or four per skater, found by
   Stats[].ComboID + combo direction + direction + button; arccos swaps Officer Dick's three for
   Private Carrera's. The tricks scored by code past the table reuse numbers (theirs and the
   table's), so those share a repeat count. No source names survive; these are the game's text. */
enum ETrick {
	TRICK_NONE = 0,

	/* flips (Flip + direction) */
	TRICK_360_SHOVE_IT = 1,       /* up */
	TRICK_IMPOSSIBLE = 2,         /* down */
	TRICK_KICKFLIP = 4,           /* left */
	TRICK_KICKFLIP_TO_INDY = 5,   /* up-left */
	TRICK_FLIP_DOWN_LEFT = 6,     /* per skater: VARIAL or SEX CHANGE */
	TRICK_HEELFLIP = 8,           /* right */
	TRICK_FLIP_UP_RIGHT = 9,      /* per skater: FINGER FLIP, HARDFLIP or VARIAL */
	TRICK_FLIP_DOWN_RIGHT = 10,   /* per skater: FRONT FOOT IMPOSSIBLE or 360 FLIP */

	/* grabs (Grab + direction) */
	TRICK_GRAB = 16,
	TRICK_JAPAN_AIR = 17,         /* up */
	TRICK_TAILGRAB = 18,          /* down */
	TRICK_METHOD = 20,            /* left */
	TRICK_MADONNA = 21,           /* up-left */
	TRICK_STALEFISH = 22,         /* down-left */
	TRICK_INDY_NOSEBONE = 24,     /* right */
	TRICK_ROCKET_AIR = 25,        /* up-right */
	TRICK_BENIHANA = 26,          /* down-right */

	/* specials, by skater (Stats[].ComboID) */
	TRICK_SPECIAL = 32,
	TRICK_HAWK_THE_900 = 32,
	TRICK_HAWK_KICKFLIP_MCTWIST = 33,
	TRICK_HAWK_540_BOARD_VARIAL = 34,
	TRICK_HAWK_360_FLIP_TO_MUTE = 35,
	TRICK_LASEK_KICKFLIP_MCTWIST = 36,
	TRICK_LASEK_FINGERFLIP_AIRWALK = 37,
	TRICK_LASEK_VARIAL_HEELFLIP_JUDO = 38,
	TRICK_BURNQUIST_BACKFLIP = 39,
	TRICK_BURNQUIST_ONE_FOOTED_SMITH = 40,
	TRICK_BURNQUIST_BURNTWIST = 41,
	TRICK_GLIFBERG_KICKFLIP_MCTWIST = 42,
	TRICK_GLIFBERG_CHRIST_AIR = 43,
	TRICK_GLIFBERG_FRONT_BACK_KICKFLIP = 44,
	TRICK_MUSKA_FRONTFLIP = 45,
	TRICK_MUSKA_ONE_FOOT_5_0_THUMPIN = 46,
	TRICK_MUSKA_360_SHOVE_IT_REWIND = 47,
	TRICK_ROWLEY_BACKFLIP = 48,
	TRICK_ROWLEY_DOUBLE_HARDFLIP = 49,
	TRICK_ROWLEY_DARKSLIDE = 50,
	TRICK_CAMPBELL_FRONTFLIP = 51,
	TRICK_CAMPBELL_CASPER_SLIDE = 52,
	TRICK_CAMPBELL_KICKFLIP_UNDERFLIP = 53,
	TRICK_REYNOLDS_BACKFLIP = 54,
	TRICK_REYNOLDS_HEELFLIP_TO_BLUNTSLIDE = 55,
	TRICK_REYNOLDS_TRIPLE_KICKFLIP = 56,
	TRICK_THOMAS_FRONTFLIP = 57,
	TRICK_THOMAS_ONE_FOOTED_NOSEGRIND = 58,
	TRICK_THOMAS_540_FLIP = 59,
	TRICK_STEAMER_BACKFLIP = 60,
	TRICK_STEAMER_PRIMO_GRIND = 61,
	TRICK_STEAMER_JUDO_MADONNA = 62,
	TRICK_DICK_YEEEEHAW_FRONTFLIP = 63,   /* Carrera: SOMI SPIN */
	TRICK_DICK_ASSUME_THE_POSITION = 64,  /* Carrera: HO-HO HO */
	TRICK_DICK_NECKBREAK_GRIND = 65,      /* Carrera: THE WELL HARDFLIP */

	/* scored by code, not in the table */
	TRICK_WALL_RIDE = 64,
	TRICK_FOOTPLANT = 65,         /* and FASTPLANT */
	TRICK_GRIND = 66,             /* + the grind (StartGrind's Direction, 2-8) */
	TRICK_FS_BOARDSLIDE = 68,
	TRICK_BS_BOARDSLIDE = 69,
	TRICK_5_0_GRIND = 70,
	TRICK_NOSEGRIND = 71,
	TRICK_50_50_GRIND = 72,
	TRICK_SMITH_GRIND = 73,
	TRICK_CROOKED_GRIND = 74,
	TRICK_SPIN = 74,              /* the "N OLLIE" spin on landing */
	TRICK_NOLLIE = 75,
	TRICK_BONUS = 76,             /* a bonus power-up */
	TRICK_GAP = 77,
	TRICK_CAR_PLANT = 78,
	TRICK_HANDPLANT = 79,

	TRICK_COUNT = 80
};

/* the trick table entry (`STrick Group[]`, 44 bytes, by ETrick; entries from TRICK_SPECIAL on are
   the specials, keyed by style + two button codes) */
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
