/* GLOBALS.H: the globals no one module owns (the original had a GLOBALS.H too). */
#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

#include "game/types.h"
#include "game/vector.h"
#include "game/card.h"
#include "game/career.h"
#include "game/dbuffer.h"
#include "game/front.h"
#include "game/m3d.h"
#include "game/pad.h"
#include "game/video.h"

class CBruce;
class CCamera;
class CGPolyLine;
class CMenu;
class CNonRenderedBit;
class CPolyLine;
class CTrickOb;
class PREManager;
struct SAnimFrame;
struct SBlockHeader;

/* globals */
extern SViewport ApocalypseViewport;
extern int BackOut;
extern SAnimFrame *BigFontAnim;
extern CCamera *CameraList;
extern int Cheat_FPS;
extern uint CriticalBigHeapUsage;
extern char DAT_800a21b0[];
extern CVector DAT_800a7f28;
extern int DAT_800d169c;
extern SBlockHeader *DAT_800d1b48;
extern SBlockHeader *DAT_800d1b4c;
extern uint Db_SkyColor;
extern int DemoTimer;
extern EFileType FileType[];
extern short GameFMV_StartX;
extern int GameFrozen;
extern int GamePaused;
extern LocalText GameText;
extern SCareer GCareer;
/* game mode */
extern int GGame;
extern int GHasCareerLoaded;
extern int GHeadPart;
extern int GLevel;
extern int GNumberOfPlayers;
extern CGPolyLine *GPolyLineList;
extern SOptions GPreferences;
extern CBruce *GSkater[];
extern SVideoRestart GVideoRestart;
/* SViewport */
extern SSaveGame LastGameState;
extern SLevel Levels[];
extern int Loading;
extern CBruce *MechList;
extern int MusicStopped;
extern CNonRenderedBit *NonRenderedBitList;
extern int OT_entry;
extern SControl Pad[];
extern SDoubleBuffer *pDoubleBuffer;
extern CPolyLine *PolyLineList;
extern uchar *pPoly;
extern SPSXRegion PSXRegion[];
extern CMenu *pYesNoMenu;
extern int Redbook_XACheckSectorOnVSync;
extern int Redbook_XAModeSet;
extern int SelectedSkater;
extern int SFX_v;
extern int SFXLEVEL;
/* the directory, 40 bytes an entry (in the port PC_SaveListDir fills it with the saves' names;
   firstfile and nextfile, the PS1 card's, are stubs that find nothing) */
extern char SLOT_DIR[][40];
extern PREManager *SpoolerBlocksFileIO;
extern SSaveGame StartupGameState;
extern SStats Stats[];
extern uint SuspendDistance;
extern CTrickOb *TrickObList;
extern int TTime;
extern int UseCacheForBits;
extern int Used[];
/* (MyVSync counts it; Pause reads it as volatile) */
extern uint Vblanks;
extern int ViewportMode;
extern short WibbleTables[][64];
extern int XALEVEL;
extern uint Xblanks;
extern CVector ZeroVector;

#endif
