/* Career / game-state data (CAREER.CPP, FRONT.CPP, MECH.CPP, GLOBALS.H): the career save
   (GCareer), the options block (GPreferences / GCareer.Options), the per-skater stat table (Stats),
   the level table (Levels) and the demo's SPreferences. Plain data, no member functions.

   Retail layouts differ from the THPS2 demo's (MAIN.SYM): offsets below come from the byte-exact
   TUs (build/class_census) and the retail data tables (Stats 0x800a44fc, Levels 0x800a5398,
   GCareer 0x800a6844); names are the demo's where the field lines up. See build/hdr_notes/career.md. */
#ifndef GAME_CAREER_H
#define GAME_CAREER_H

#include "game/types.h"

/* one high-score table entry (demo layout) */
struct SHighScore {
	char Name[4]; /* 0x0 */
	uchar Skater; /* 0x4 (Stats index) */
	uint Score;   /* 0x8 */
}; /* size 0xc */

/* one skater's career progress (demo SCharacterProgress, shrunk to 14 bytes in THPS1) */
struct SSkaterCareer {
	uchar NumTapes;  /* 0x0 goals (tapes) got over the career levels */
	uchar NumBronze; /* 0x1 */
	uchar NumSilver; /* 0x2 */
	uchar NumGold;   /* 0x3 */
	uchar Tapes[10]; /* 0x4 per level: goal bits 0..4, medal bits above (Career_GiveMedal) */
}; /* size 0xe */

/* the options block (demo SOptions; retail OtherOptions grew to 66 ints, MedalMessagesUsed
   shrank to one int per skater). GPreferences (demo type GamePreferences, which only wraps
   this) and GCareer.Options are both SOptions. */
struct SOptions {
	int SFXVolume;                 /* 0x000 */
	int MusicVolume;               /* 0x004 */
	int OtherOptions[66];          /* 0x008 */
	int AutoKickEnabled[2];        /* 0x110 */
	int VibrationEnabled[2];       /* 0x118 */
	int ControllerFunction[2][4];  /* 0x120 */
	char LastHighScoreInitials[4]; /* 0x140 */
	int TrickTipUsed[2];           /* 0x144 */
	int TrickTipMask[2];           /* 0x14c */
	int MedalMessagesUsed[11];     /* 0x154 per skater, masked with the competition's medal mask */
	int unused0;                   /* 0x180 */
	int TrickTipsEnabled;          /* 0x184 */
	int ScoreDisplayEnabled;       /* 0x188 */
	int HorseLen;                  /* 0x18c */
	char HorseWord[16];            /* 0x190 */
}; /* size 0x1a0 */

/* the career save (GCareer, 0x800a6844); also the memory-card save image */
struct SCareer {
	int Version;                         /* 0x000 (0x60299) */
	int MaxTapes;                        /* 0x004 */
	SSkaterCareer CharacterProgress[11]; /* 0x008 one per Stats entry */
	char pada2[0xa4 - 0xa2];
	SHighScore HighScore[20][8]; /* 0x0a4 [level][rank] */
	char LastSkater[2];          /* 0x824 per player */
	char LastDeck[2];            /* 0x826 */
	char LastWheel[2];           /* 0x828 */
	char LastTrucks[2];          /* 0x82a */
	uchar LastLevel;             /* 0x82c */
	ushort TimeStamp;            /* 0x82e */
	SOptions Options;            /* 0x830 */
}; /* size 0x9d0 */

/* a board choice in a skater's Stats entry */
struct SBoard {
	char *File;   /* 0x00 texture (demo BoardTexture) */
	char *Name;   /* 0x04 (demo BoardName) */
	uchar Req;    /* 0x08 tapes to unlock (demo TapesToUnlock) */
	int Stats[5]; /* 0x0c ollie, speed, air, balance, turning (demo names) */
}; /* size 0x20 */

/* a player's current board stats (GBoardStats[2]) */
struct BoardStats {
	uchar s[5]; /* ollie, speed, air, balance, turning (demo names) */
}; /* size 0x5 */

/* one skater (Stats[], 0x800a44fc; 11 entries) */
struct SStats {
	char *PSXName;    /* 0x000 model file ("hawk") */
	char *PSXName2;   /* 0x004 two-player model ("burnq_2p") */
	char *FrontName;  /* 0x008 front-end model ("hawk_fe") */
	int ComboID;      /* 0x00c */
	uchar Board;      /* 0x010 */
	uchar Wheel1;     /* 0x011 */
	uchar Wheel2;     /* 0x012 */
	uchar Head;       /* 0x013 */
	signed char Neck; /* 0x014 (signed: retail loads it with lb; Psy-Q's plain char is unsigned) */
	char pad15[3];
	int Stats[5];       /* 0x018 */
	char *Name;         /* 0x02c "Tony Hawk" */
	char *pBio[6];      /* 0x030 age, country, home town, years pro, stance, height */
	uint BoardChecksum; /* 0x048 */
	uint WheelChecksum; /* 0x04c */
	uint TruckChecksum; /* 0x050 */
	SBoard Boards[8];   /* 0x054 */
}; /* size 0x154 */

/* one level (Levels[], 0x800a5398; 10 entries) */
struct SLevel {
	char *pLevelName;       /* 0x00 "Warehouse" */
	char *pCityName;        /* 0x04 "Woodland Hills" */
	char *pVab;             /* 0x08 "ware" */
	int isCompetition;      /* 0x0c */
	int isLocked;           /* 0x10 */
	char *pTRGName;         /* 0x14 "SkWare_T" */
	uint GoldScore;         /* 0x18 */
	uint SilverScore;       /* 0x1c */
	uint BronzeScore;       /* 0x20 */
	char *pImageName;       /* 0x24 "levsel0.bmp" */
	int map_loc[2];         /* 0x28 */
	char *pLoadImage;       /* 0x30 "load_war.bmp" */
	char *pBackImage;       /* 0x34 "back_war.bmp" */
	char *pGoalName;        /* 0x38 "boxes" (the collect-5 goal) */
	int TapesToUnlock;      /* 0x3c */
	int MedalsToUnlock;     /* 0x40 */
	int HiScore;            /* 0x44 tape score 1 / high-score table base */
	int ProScore;           /* 0x48 tape score 2 */
	int Crowd;              /* 0x4c */
	int ExtraPolyBuffer[2]; /* 0x50 by number of players - 1 */
	int TrickTipMask[2];    /* 0x58 */
	int FMV;                /* 0x60 */
	int IntroStartPosX;     /* 0x64 */
	int IntroStartPosY;     /* 0x68 */
	int IntroStartPosZ;     /* 0x6c */
	short IntroStartAngX;   /* 0x70 */
	short IntroStartAngY;   /* 0x72 */
	short IntroStartAngZ;   /* 0x74 */
}; /* size 0x78 */

/* demo layout (Front_SavePreferences(SPreferences &) is an empty stub in retail); part of SSaveGame */
struct SPreferences {
	int Difficulty;             /* 0x00 */
	int SFXLevel;               /* 0x04 */
	int MidiLevel;              /* 0x08 */
	int XALevel;                /* 0x0c */
	uchar DigitalSelectWeapon;  /* 0x10 */
	uchar DigitalCrouch;        /* 0x11 */
	uchar DigitalJump;          /* 0x12 */
	uchar DigitalSmartBomb;     /* 0x13 */
	uchar AnalogueSelectWeapon; /* 0x14 */
	uchar AnalogueCrouch;       /* 0x15 */
	uchar AnalogueJump;         /* 0x16 */
	uchar AnalogueSmartBomb;    /* 0x17 */
}; /* size 0x18 */

/* functions */
void Options_Init();
int Career_Got(int Goal);
void Career_GiveTape(int Level, int Goal);
int Career_LevelTapes(int Level);
/* medal kinds: callers pass 5/6/7 for gold/silver/bronze (competition/winlosedisplay); only the
   type is known, for the mangled names */
enum EMedalType { MEDAL_NONE };

void Career_Init();
void Options_SetPreferences();
void Options_GetPreferences();
void Career_GiveMedal(int Level, EMedalType medal);

/* globals */
extern int GLoadedCareerIsSaved;
#endif
