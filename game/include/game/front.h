/* Front-end menus and text (FRONT.CPP): CMenu and its SEntry lines, the GameText string table
   (LocalText), and small structs local to the front-end screens.

   Not here: SHighlight (replay highlights, game/video.h); the front-end "CItem" base is CClass
   (see game/shellutils.h); SFront (mech/ctor_cbruce) is a stand-in for CCamera; SText
   (front/controller*) and SFrontStrings (rfront/memcardupdate) are stand-ins for LocalText.
   See build/hdr_notes/front.md. */
#ifndef GAME_FRONT_H
#define GAME_FRONT_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/class.h"
#include "game/career.h"
class CTape;
struct SAnimFrame;

/* one line of a CMenu */
struct SEntry {
	char *pString;       /* 0x00 */
	short CurrentScale;  /* 0x04 */
	short TargetScale;   /* 0x06 */
	short HiScale;       /* 0x08 */
	short LowScale;      /* 0x0a */
	uchar Clearance;     /* 0x0c */
	uchar Visible;       /* 0x0d */
	uchar Enabled;       /* 0x0e */
	uchar HiR;           /* 0x0f */
	uchar HiG;           /* 0x10 */
	uchar HiB;           /* 0x11 */
	uchar LowR;          /* 0x12 */
	uchar LowG;          /* 0x13 */
	uchar LowB;          /* 0x14 */
	uchar FadeUpCounter; /* 0x15 */
	                     /* sizeof(SEntry) == 0x18 */
};

/* vtable 0x8009e538 */
class CMenu : public CClass {
  public:
	uchar mJustification; /* 0x004 */
	uchar mRep;           /* 0x005 */
	uchar mLine;          /* 0x006 */
	uchar mHasCursor;     /* 0x007 */
	uchar mCursorLine;    /* 0x008 */
	uchar mNumLines;      /* 0x009 */
	char pad0a[0x0c - 0x0a];
	int mX;            /* 0x00c */
	int mY;            /* 0x010 */
	int mLineSep;      /* 0x014 */
	SEntry mEntry[15]; /* 0x018 */
	/* sizeof(CMenu) == 0x180 */

	CMenu(int x, int y, uchar justification, int hiScale, int lowScale, int lineSep);
	virtual ~CMenu(); /* FUN_; demo _._5CMenu */
	void CentreY();
	void SetLine(uchar Line);
	int FindEntry(const char *pString);
	void AddEntry(const char *pString);
	void EntryOn(const char *pString);
	void EntryOff(const char *pString);
	int IsEntryOn(const char *pString);
	int ChoiceIs(const char *pString);
	void Display();
	void Update();
};

/* The localised front-end strings (global `GameText`, 0x800a5d5c). Demo struct LocalText (0x28c),
   checked against the retail table: identical up to +0x178; THPS1 drops the demo's
   pPressXToContinue/pNextPlayer (after pReplayInstructions) and pCardFullWarning1..5, and adds
   pCardError ("memory card error") at +0x244. */
struct LocalText {
	char *pLevelHints;               /* 0x000 */
	char *pEndGame;                  /* 0x004 */
	char *pEndRun;                   /* 0x008 */
	char *pNoVideoFiles;             /* 0x00c */
	char *pNoCareerFiles;            /* 0x010 */
	char *pSaveCareerQM;             /* 0x014 */
	char *pAFileByThat;              /* 0x018 */
	char *pNameAlreadyExists;        /* 0x01c */
	char *pSKATETape;                /* 0x020 */
	char *p20000Tape;                /* 0x024 */
	char *p40000Tape;                /* 0x028 */
	char *p5GoalsTape;               /* 0x02c */
	char *pHiddenTape;               /* 0x030 */
	char *pEnterName;                /* 0x034 */
	char *pPressStart;               /* 0x038 */
	char *pCareerMode;               /* 0x03c */
	char *pSingleSession;            /* 0x040 */
	char *pTwoPlayers;               /* 0x044 */
	char *pPractice;                 /* 0x048 */
	char *pAutoKick;                 /* 0x04c */
	char *pOn;                       /* 0x050 */
	char *pOff;                      /* 0x054 */
	char *pOllie;                    /* 0x058 */
	char *pFullReplay;               /* 0x05c */
	char *pHighlights;               /* 0x060 */
	char *pSaveVideo;                /* 0x064 */
	char *pLoadVideo;                /* 0x068 */
	char *pExit;                     /* 0x06c */
	char *pSaveCareer;               /* 0x070 */
	char *pRetry;                    /* 0x074 */
	char *pRestart;                  /* 0x078 */
	char *pSelectLevel;              /* 0x07c */
	char *pEquipmentSelect;          /* 0x080 */
	char *pWatchReplay;              /* 0x084 */
	char *pMainMenu;                 /* 0x088 */
	char *pYouGotATape;              /* 0x08c */
	char *pYouGotNTapes;             /* 0x090 */
	char *pStatistics;               /* 0x094 */
	char *pTotalScore;               /* 0x098 */
	char *pHighScore;                /* 0x09c */
	char *pTapesCollected;           /* 0x0a0 */
	char *pTricksLanded;             /* 0x0a4 */
	char *pBails;                    /* 0x0a8 */
	char *pBestCombo;                /* 0x0ac */
	char *pNone;                     /* 0x0b0 */
	char *pBestComboNTricksNPoints;  /* 0x0b4 */
	char *pBestComboNTrickNPoints;   /* 0x0b8 */
	char *pAreYouSure;               /* 0x0bc */
	char *pCarPlant;                 /* 0x0c0 */
	char *pNextHeat;                 /* 0x0c4 */
	char *pEndComp;                  /* 0x0c8 */
	char *pLoadComplete;             /* 0x0cc */
	char *pSaveComplete;             /* 0x0d0 */
	char *pLoadFailed;               /* 0x0d4 */
	char *pSaveFailed;               /* 0x0d8 */
	char *pGrindSlide;               /* 0x0dc */
	char *pGrabTrick;                /* 0x0e0 */
	char *pFlipTrick;                /* 0x0e4 */
	char *pFlipTrickKick;            /* 0x0e8 */
	char *pJumpOllie;                /* 0x0ec */
	char *pBooleanOn;                /* 0x0f0 */
	char *pBooleanOff;               /* 0x0f4 */
	char *pTrickTips;                /* 0x0f8 */
	char *pScoreDisplay;             /* 0x0fc */
	char *pVibration;                /* 0x100 */
	char *pHiResUnformatted1;        /* 0x104 */
	char *pHiResUnformatted2;        /* 0x108 */
	char *pHiResUnformatted3;        /* 0x10c */
	char *pHiResFormatting;          /* 0x110 */
	char *pHiResFormatFailed1;       /* 0x114 */
	char *pHiResFormatFailed2;       /* 0x118 */
	char *pHiResFormatFailed3;       /* 0x11c */
	char *pHiResFormatComplete;      /* 0x120 */
	char *pReturnToCompetition;      /* 0x124 */
	char *pHiResScroll;              /* 0x128 */
	char *pHiResUp;                  /* 0x12c */
	char *pHiResDown;                /* 0x130 */
	char *pHiResUpOrDown;            /* 0x134 */
	char *pBlueSteals;               /* 0x138 */
	char *pRedSteals;                /* 0x13c */
	char *pHighScores;               /* 0x140 */
	char *pHorsePlayTo;              /* 0x144 */
	char *pHorseChangeWord;          /* 0x148 */
	char *pChangeSoundLevels;        /* 0x14c */
	char *pSFXLevel;                 /* 0x150 */
	char *pMusicLevel;               /* 0x154 */
	char *pSkipTrack;                /* 0x158 */
	char *pBackToMainMenu;           /* 0x15c */
	char *pYouGetTheLetter;          /* 0x160 */
	char *pUpdateSaveFile;           /* 0x164 */
	char *pOutOfBounds1;             /* 0x168 */
	char *pOutOfBounds2;             /* 0x16c */
	char *pOutOfBounds3;             /* 0x170 */
	char *pOutOfBounds4;             /* 0x174 */
	char *pReplayInstructions;       /* 0x178 */
	char *pPaused;                   /* 0x17c */
	char *pContinue;                 /* 0x180 */
	char *pSave;                     /* 0x184 */
	char *pQuit;                     /* 0x188 */
	char *pPleaseReinsertController; /* 0x18c */
	char *pEnterCheat;               /* 0x190 */
	char *pToGoBack;                 /* 0x194 */
	char *pToSelect;                 /* 0x198 */
	char *pToAccept;                 /* 0x19c */
	char *pLevelSelect;              /* 0x1a0 */
	char *pSkipToRestart;            /* 0x1a4 */
	char *pAccept;                   /* 0x1a8 */
	char *pCancel;                   /* 0x1ac */
	char *pPressAnyButton;           /* 0x1b0 */
	char *pYes;                      /* 0x1b4 */
	char *pNo;                       /* 0x1b8 */
	char *pReallyQuit;               /* 0x1bc */
	char *pOptions;                  /* 0x1c0 */
	char *pEmpty;                    /* 0x1c4 */
	char *pPleaseInsert1;            /* 0x1c8 */
	char *pPleaseInsert2;            /* 0x1cc */
	char *pPleaseInsert3;            /* 0x1d0 */
	char *pPleaseInsert4;            /* 0x1d4 */
	char *pUnformatted1;             /* 0x1d8 */
	char *pUnformatted2;             /* 0x1dc */
	char *pUnformatted3;             /* 0x1e0 */
	char *pUnformatted4;             /* 0x1e4 */
	char *pSureWishToFormat1;        /* 0x1e8 */
	char *pSureWishToFormat2;        /* 0x1ec */
	char *pSureWishToFormat3;        /* 0x1f0 */
	char *pFormatting;               /* 0x1f4 */
	char *pDoNotRemove1;             /* 0x1f8 */
	char *pDoNotRemove2;             /* 0x1fc */
	char *pCheckingCard;             /* 0x200 */
	char *pReadingCard;              /* 0x204 */
	char *pCardFull1;                /* 0x208 */
	char *pCardFull2;                /* 0x20c */
	char *pCardFull3;                /* 0x210 */
	char *pCardFull4;                /* 0x214 */
	char *pCardFull5;                /* 0x218 */
	char *pCardFull6;                /* 0x21c */
	char *pSelectSaveSlot1;          /* 0x220 */
	char *pSelectSaveSlot2;          /* 0x224 */
	char *pOKToOverwrite1;           /* 0x228 */
	char *pOKToOverwrite2;           /* 0x22c */
	char *pOKToOverwrite3;           /* 0x230 */
	char *pSaving;                   /* 0x234 */
	char *pWriteFailed;              /* 0x238 */
	char *pFormatFailed;             /* 0x23c */
	char *pFormatComplete;           /* 0x240 */
	char *pCardError;                /* 0x244 (THPS1-only) */
	char *pSelectGameToLoad;         /* 0x248 */
	char *pInsertCardWithSavedGame1; /* 0x24c */
	char *pInsertCardWithSavedGame2; /* 0x250 */
	char *pInsertCardWithSavedGame3; /* 0x254 */
	char *pInsertCardWithSavedGame4; /* 0x258 */
	char *pTPickup;                  /* 0x25c */
	char *pTapePickup;               /* 0x260 */
	char *pEPickup;                  /* 0x264 */
	char *pSPickup;                  /* 0x268 */
	char *pKPickup;                  /* 0x26c */
	char *pAPickup;                  /* 0x270 */
	                                 /* sizeof(LocalText) == 0x274 */
};

/* the skater-select stat rows (front/skaterselect1p: SStatRow; front/skaterselect2p: SStatBar).
   Function-local arrays; same 24-byte shape, names as each TU had them. */
struct SStatRow {
	int x;     /* 0x00 */
	int y;     /* 0x04 */
	int value; /* 0x08 */
	int m0c;   /* 0x0c */
	int m10;   /* 0x10 */
	int bar;   /* 0x14 */
};

struct SStatBar {
	int x;     /* 0x00 */
	int y;     /* 0x04 */
	int value; /* 0x08 */
	int m0c;   /* 0x0c */
	int m10;   /* 0x10 */
	int type;  /* 0x14 */
};

/* front/controlleroptions: the eight button-function labels, copied as a block from 0x800a1d0c */
struct SLabels {
	char *Name[8];
};

/* the paused front end's screens (Status) */
enum EPausedStatus {
	STATUS_NORMAL,
	STATUS_PAUSEDMENU,
	STATUS_SKIPTORESTART,
	STATUS_REALLYQUIT,
	STATUS_PADUNPLUGGED,
	STATUS_SAVEGAME,
	STATUS_QUITSAVE,
	STATUS_MEDALSAVE,
	STATUS_ENDLEVELFADESCREEN,
	STATUS_DOSAVEFADESCREEN,
	STATUS_NOSAVEFADESCREEN,
	STATUS_GOTTAPE,
	STATUS_TAPESAVE,
	STATUS_STATS,
	STATUS_ENDINGSCREEN,
	STATUS_VIDEOSCREEN,
	STATUS_VIDEOSCREEN2,
	STATUS_REALLYMAINMENU,
	STATUS_MEMCARDMENUS,
	STATUS_ENDHEAT,
	STATUS_ENDHEATREPLAY,
	STATUS_ENDHEATJUDGES,
	STATUS_ENDHEATLADDER,
	STATUS_ENDHEATWINLOSE,
	STATUS_ENDHEATMENU,
	STATUS_ENDHEATVIDEOMENU,
	STATUS_BEGINHEAT,
	STATUS_ENDGAME,
	STATUS_ENDGAMEALLGOLD,
	STATUS_ENDGAMEPLAYERMOVIE,
	STATUS_ENDGAMEBAILMOVIE,
	STATUS_ENDGAMESAVE,
	STATUS_ENDGAMECREDITS,
	STATUS_STARTHORSE,
	STATUS_CHANGEHORSEWORD,
	STATUS_LOADSUCCESS,
	STATUS_LOADFAILED,
	STATUS_HIGHSCORE,
	STATUS_SETSOUNDLEVELS
};
enum EMemCardState { SVSTATUS_CHECKINGCARDPAUSE };
enum ESaveAction { SAVE_VIDEO, SAVE_GAME };

/* functions */
struct SSaveGame;
void Horse_Reset();
void Horse_DisplayWords(int *p);
int InCompetition();
void Front_ClearScreen();
void Front_Init();
void Front_PrintGoBackSelect();
void Front_PrintCentredPrompt(const char *s);
void Front_Display();
void Front_GetButtons(int &a, int &b, int &c, int &d);
void RestartCompFrig();
void Front_SaveGameState();
void Front_SetPreferences(const SPreferences &prefs);

void Front_PrintDemo();
void Front_DisplayScreen();
void PrintPaused();
void CommonSwitchOnMenuCode();
void Front_SwitchOnEndHeat();
void Front_SwitchOnEndLevelMenu();
void Front_SwitchOnStartLevelMenu();
void Front_PrintHorseLetters();
void Front_SwitchOnVideoScreen();
void Front_Update();
void Front_LoadGame(SSaveGame *p);

/* globals */
extern SAnimFrame *ButtonsAnim;
extern int Cheat_BigHead;
extern int Cheat_LevelSelect;
extern int Cheat_RestartSkip;
extern int CurrentGameLevel;
extern int DAT_800d1cb8;
extern char DemoName[];
/* picture y by screen */
extern uint GameFade;
extern int GAutoGame;
extern int GAutoLevel;
extern int GAutoPlayer1;
extern int GAutoPlayer2;
extern int GAutoTest;
extern BoardStats GBoardStats[];
extern int GHeadUp;
extern int GHighTags1;
extern int GHighTags2;
extern int GHorseClearInput2;
extern int GHorseDoneStart;
extern int GHorseLetters[];
extern int GHorseRestart;
extern int GHorseWordChosen;
extern int GLosses1;
extern int GLosses2;
extern int GRollCredits;
extern int GSoftResetEnabled;
extern int GState;
extern int GStreak1;
extern int GStreak2;
extern CTape *GTape[];
extern int GWins1;
extern int GWins2;
extern int SelectedSkater2;
extern int SizeUp;
extern int StartScreen;
extern EPausedStatus Status;
#endif
