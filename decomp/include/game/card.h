/* Memory card (CARD.CPP) and the demo's saved-game structs (FRONT.CPP: StartupGameState /
   LastGameState). Plain data, no member functions. Layouts are the demo's (MAIN.SYM); the retail
   sizes agree (SCardHead 0x200 in Card_ReadFileHeader's callers, SSaveGame 0x88 between
   LastGameState 0x800a5bdc and StartupGameState 0x800a5c64). See build/hdr_notes/card.md. */
#ifndef GAME_CARD_H
#define GAME_CARD_H

#include "game/types.h"
#include "game/career.h"

/* Card_Write / Card_Load */
enum EFileType { THPS_SAVE_FILE, THPS_VIDEO_FILE, THPS_VIDEO_HEADER, OTHER_FILE };

/* the PS1 memory-card file header (title frame + icon frames) */
struct SCardHead {
	char Magic[2];      /* 0x000 "SC" */
	char Type;          /* 0x002 icon frame count/type */
	char BlockEntry;    /* 0x003 */
	char Title[64];     /* 0x004 Shift-JIS */
	char reserve[28];   /* 0x044 */
	char Clut[32];      /* 0x060 */
	char Icon[3][128];  /* 0x080 */
};                      /* size 0x200 */

/* demo layout (leftover from Neversoft's earlier game); THPS1 never reads its fields by name */
struct SGameState {
	char TrigFileName[9];  /* 0x00 */
	char RestartName[50];  /* 0x09 */
	uchar pad1;            /* 0x3b */
	uchar pad2;            /* 0x3c */
	uchar pad3;            /* 0x3d */
	uchar WeaponsGot;      /* 0x3e */
	uchar CurrentWeapon;   /* 0x3f */
	int Ammo[8];           /* 0x40 */
	int Health;            /* 0x60 */
	int SmartBombs;        /* 0x64 */
	int Lives;             /* 0x68 */
};                         /* size 0x6c */

/* StartupGameState / LastGameState; Front_CalculateGameChecksum(SSaveGame &) */
struct SSaveGame {
	uint Checksum;            /* 0x00 */
	SGameState State;         /* 0x04 */
	SPreferences Preferences; /* 0x70 */
};                            /* size 0x88 */

#endif
