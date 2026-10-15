/* Front_Main__Fv (0x8005bb70) */
/* C++ (front end). The front-end screen dispatcher, called once from FUN_8006b3f8 (after the boot
   init) and never returning. No demo twin (THPS2's FrontEnd2_Main is a different, table-driven
   design). Each pass:
     - (re)start: on a soft reset (START+SELECT, DAT_800d1cb8) unload everything and clear it; set
       DemoName to "demoA.rec" and clear the video-restart data.
     - top: in autotest mode (GAutoTest) cycle level/skater/players and launch straight away.
       Otherwise, coming back from a game (StartScreen = 1..6) jump straight back into the screen
       the player left from; else run the title screen (Front_TitleScreen) and then the main menu
       (Front_MainMenu).
     - main-menu choice: 3/4/17/18/19 = 1-player game modes -> skater select (Front_SkaterSelect1P);
       33/34/35/1234567 = 2-player modes -> 2P skater select (Front_SkaterSelect2P); 1553/1554 = replay
       the 1P/2P video restart; 99, 1569-1574, 25202-25207 = FMV movies; 100/101 = credits.
     - 1P: skater select -> board select (Front_BoardSelect1P); 2P: skater select -> board select
       (Front_BoardSelect2P); both then level select (LevelSelect_Run). -1 from a screen backs up one.
     - launch: record the choice, start the game (kickIt -> LaunchGame), tear down, reload
       the shell, and loop (rolling the credits first if the career was finished).
   Register levers: the duplicated case tails (2P modes, return screens 3/4/5) are cross-jumped in
   retail but their extra refs are what give `players`/`menu` their s1/s4 priority; the demo load
   sits in a one-pass do{}while(0) (macro-like), which gives DemoName's hoisted %hi s7 over DB's fp.
   The whole dispatcher is a goto loop (`restart:` ... `goto restart`), not for(;;): with no loop
   notes the pseudos' refs aren't loop-weighted, so local-alloc gives the ++GAutoLevel == 9
   counter v0 ahead of the short-lived constant 9 (as retail). The Competition_Init test is the `&&`
   form `!GVideoReplay && Levels[choice].isCompetition == 1`, which expands as (base+12)+idx and
   gives retail's base-first `addu base,base,idx`. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/video.h"
#include "game/career.h"
#include "game/m3d.h"
#include "game/mess.h"
#include "game/gamefmv.h"
#include "game/mem.h"
#include "game/card.h"
#include "game/credits.h"
#include "game/db.h"
#include "game/front.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/levelselect.h"
#include "game/sfx.h"
#include "game/shellutils.h"

enum EPackInitType { PACK_0, PACK_1 };

struct SName {
	char c[10];
};

void Front_ClearGVideoRestartData();
void Mess_LoadFont(char *, int, int, int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetFontScale(unsigned);
void Mess_SetCurrentFont(char *);
void Mess_UnloadFont();
void Pack_DefragmentFully();
void Init_Restart(EPackInitType, int);
void Competition_Init();

extern "C" {
	extern SVideoRestart GVideoRestart;   /* GVideoRestart */
	extern SLevel Levels[];               /* Levels */
	extern SCareer GCareer;
	extern SDoubleBuffer DoubleBuffer[2]; /* DB */
	extern SName DemoName;                /* DemoName */
	extern int GRollCredits;              /* GRollCredits */
	extern int GGame;                     /* GGame */
	extern int GAutoTest;                 /* autotest */
	extern int GAutoPlayer1;
	extern int GAutoPlayer2;
	extern int GAutoLevel;
	extern int GAutoGame;
	extern int SelectedSkater2;  /* 2nd player's skater */
	extern int GNumberOfPlayers; /* GNumberOfPlayers */
	extern int DAT_800d1d80;
	extern int DAT_800d1dc0;
	extern int GVideoReplay; /* GVideoReplay */
	extern int GLevel;       /* GLevel */

	extern int StartScreen; /* screen to return to */
	extern short DAT_800d1ca0;
	extern int DemoTimer; /* DemoTimer */
	extern int DAT_800d1cb4;
	extern int DAT_800d1cb8;
}

static inline int GetReturnScreen() {
	int s = StartScreen;

	StartScreen = 5;
	return s;
}

void Front_Main(void) {
	int choice;
	int players;
	int mode;
	int skater;
	int menu;
	int r;

	ClearSoftReset();
restart:
	if (DAT_800d1cb8) {
	reset:
		Mess_UnloadAllFonts();
		FrontLoader_UnloadAll();
		Front_FreeSpritesPRE();
		ClearSoftReset();
	}

	DemoName = *(SName *)"demoA.rec";
	DAT_800d1cb4 = 1;
	Front_ClearGVideoRestartData();
	Career_Init();
	Front_DisplayScreen();
top:
	FrontLoader_Reset();

	DAT_800d1dc0 = 1;
	GNumberOfPlayers = 1;
	DemoTimer = 1600;
	PadBuffer_SetNumPlayers(2);

	DAT_800d1ca0 = 0;

	if (GAutoTest) {
		mode = 1;
		skater = GAutoPlayer1;
		choice = GAutoLevel;
		SelectedSkater2 = GAutoPlayer2;

		if (!GAutoGame) {
			GNumberOfPlayers = 1;
			GGame = 1;
			players = 1;
		} else {
			players = 2;
			GNumberOfPlayers = 2;
			GGame = 8;
		}

		if (++GAutoPlayer1 == 11)
			GAutoPlayer1 = 0;

		if (++GAutoLevel == 9) {
			GAutoLevel = 0;
			GAutoGame ^= 1;

			if (++GAutoPlayer2 == 11)
				GAutoPlayer2 = 0;
		}

		goto launch;
	}

	Front_LoadSpritesPRE();

	if (StartScreen) {
		Front_SetMusicLoop(157);
		Mess_LoadFont("mainf.fnt", -1, -1, -1);
		Mess_SetRGB(128, 128, 128, 0);
		Mess_SetFontScale(256);
		Mess_SetTextJustify(1);
		Mess_SetFontShadowOffset(4);

		mode = 0;
		skater = (unsigned char)GCareer.LastSkater[0];
		players = 1;

		switch (GGame) {
			case 1:
				menu = 16;
				break;
			case 2:
			case 3:
				mode = 1;
				break;
			case 4:
				break;
			case 5:
			case 6:
				mode = 1;
				menu = 32;
				players = 2;
				break;
			case 7:
				mode = 5;
				menu = 32;
				players = 2;
				break;
			case 8:
				mode = 3;
				menu = 32;
				players = 2;
				break;
		}

		switch (GetReturnScreen()) {
			case 2:
				if (players != 1)
					goto board2;

				PadBuffer_SetNumPlayers(1);
				goto board1;
			case 1:
				if (players == 1)
					PadBuffer_SetNumPlayers(1);

				goto level;
			case 3:
				GVideoRestart.NumSkaters = 0;
				menu = 96;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 4:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 5:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 6:
				if (players == 1)
					PadBuffer_SetNumPlayers(1);

				choice = GLevel;
				Mess_SetCurrentFont("mainf.fnt");
				Mess_UnloadFont();
				Front_FreeSpritesPRE();
				goto launch;
		}
	}

	StartScreen = 5;
title:
	FrontLoader_UnloadAll();
	Front_TitleScreen();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont("Start_1.fnt");
	Mess_UnloadFont();

	if (!DemoTimer)
		goto demo;

	menu = 0;
mainmenu:
	Front_SetMusicLoop(157);
	Pack_DefragmentFully();

	choice = Front_MainMenu(menu);

	if (DAT_800d1cb8)
		goto reset;

	if (!DemoTimer)
		goto demo;

	Options_SetPreferences();

	mode = 0;
	players = 1;
	menu = 0;

	switch (choice) {
		case -1:
			goto title;
		case 17:
		case 18:
			if (choice == 18) {
				DAT_800d1d80 = 0;
				Career_Init();
			}

			GGame = players;
			menu = 16;
			goto skater1;
		case 3:
			GGame = choice;
			mode = 1;
			goto skater1;
		case 4:
			GGame = 2;
			mode = 1;
			goto skater1;
		case 34:
			mode = 1;
			GGame = 5;
			menu = 32;
			players = 2;
			goto skater2;
		case 33:
			mode = 3;
			GGame = 8;
			menu = 32;
			players = 2;
			goto skater2;
		case 1234567:
			mode = 1;
			GGame = 6;
			menu = 32;
			players = 2;
			goto skater2;
		case 35:
			mode = 5;
			GGame = 7;
			menu = 32;
			players = 2;
			goto skater2;
		case 1553:
			mode = 2;
			choice = GVideoRestart.Level;
			skater = GVideoRestart.Skater;
			StartScreen = 3;
			GGame = GVideoRestart.GGame;
			goto replay;
		case 1554:
			players = 2;
			mode = 2;
			choice = GVideoRestart.Level;
			skater = GVideoRestart.Skater2;
			StartScreen = 3;
			GGame = GVideoRestart.GGame;
			SelectedSkater2 = GVideoRestart.Skater;
		replay:
			FrontLoader_UnloadAll();
			Front_FreeSpritesPRE();
			goto launch;
		case 19:
			DAT_800d1d80 = players;
			GGame = players;
			menu = 16;
			goto skater1;
		case 100:
		case 101:
			goto credits;
	}

	if ((unsigned)(choice - 99) < 2 || (unsigned)(choice - 1569) < 6 ||
	    (unsigned)(choice - 25202) < 6)
		goto movie;

	goto title;

skater1:
	PadBuffer_SetNumPlayers(1);

	GNumberOfPlayers = 1;
	skater = Front_SkaterSelect1P();

	if (DAT_800d1cb8)
		goto reset;

	if (skater == -1) {
		PadBuffer_SetNumPlayers(2);
		goto mainmenu;
	}

	goto board1;
skater2:
	Pack_DefragmentFully();

	skater = Front_SkaterSelect2P();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont("stats2.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("mainf.fnt");

	if (skater == -1)
		goto mainmenu;

	goto board2;
board1:
	r = Front_BoardSelect1P(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater1;

	goto level;
board2:
	r = Front_BoardSelect2P(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater2;
level:
	choice = LevelSelect_Run(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (choice == -1) {
		if (players == 1)
			goto board1;

		if (players == 2)
			goto board2;
	}

	Mess_SetCurrentFont("mainf.fnt");
	Mess_UnloadFont();
	Front_FreeSpritesPRE();
launch:
	Mess_UnloadAllFonts();
	FrontLoader_UnloadAll();
	LoadingScreen_SetCurrentLevel(choice, mode, skater);

	GNumberOfPlayers = players;
	DAT_800d1dc0 = 0;

	if (!GVideoReplay && Levels[choice].isCompetition == 1)
		Competition_Init();

	Front_StopMusicLoop();
	kickIt(choice, skater);
	Init_Restart(PACK_0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	HiRes_Init(0);
	Front_DisplayScreen();
	SFX_Init("shell.vab");
	Front_Init();

	if (DAT_800d1cb8) {
		StartScreen = 0;
		goto restart;
	}

	if (!GRollCredits)
		goto top;

	GRollCredits = 0;
	choice = 101;
	goto credits;

demo:
	do {
		Card_LoadDemoReplay((const char *)&DemoName);
	} while (0);

	mode = 2;
	menu = 16;
	players = GVideoRestart.NumSkaters;
	skater = GVideoRestart.Skater;
	choice = GVideoRestart.Level;
	SelectedSkater2 = GVideoRestart.Skater2;
	GGame = GVideoRestart.GGame;
	Front_FreeSpritesPRE();

	StartScreen = 4;
	goto launch;

movie:
	HiRes_BlankScreen();

	DAT_800d1dc0 = 0;
	Front_StopMusicLoop();

	if ((unsigned)(choice - 99) < 2)
		menu = 96;
	else if ((unsigned)(choice - 1569) < 6)
		menu = 1568;
	else if ((unsigned)(choice - 25202) < 6)
		menu = 25200;

	switch (choice) {
		case 99:
			GameFMV_PlayMovie(-26);
			break;
		case 1569:
			GameFMV_PlayMovie(-25);
			break;
		case 1570:
			GameFMV_PlayMovie(-12);
			break;
		case 1571:
			GameFMV_PlayMovie(-13);
			break;
		case 1572:
			GameFMV_PlayMovie(-14);
			break;
		case 1573:
			GameFMV_PlayMovie(-15);
			break;
		case 1574:
			GameFMV_PlayMovie(-16);
			break;
		case 25202:
			GameFMV_PlayMovie(-17);
			break;
		case 25203:
			GameFMV_PlayMovie(-18);
			break;
		case 25204:
			GameFMV_PlayMovie(-19);
			break;
		case 25205:
			GameFMV_PlayMovie(-20);
			break;
		case 25206:
			GameFMV_PlayMovie(-21);
			break;
		case 25207:
			GameFMV_PlayMovie(-11);
			break;
		case 100:
			break;
	}

	if (DoubleBuffer[0].Polys)
		Mem_Delete(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	HiRes_Init(1);
	FrontLoader_SetFiles(0);
	Front_DisplayScreen();
	goto mainmenu;

credits:
	Front_SetMusicLoop(choice == 101 ? 157 : -1);
	Credits_Roll(choice != 100, 1);

	if (DoubleBuffer[0].Polys)
		Mem_Delete(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	HiRes_Init(1);
	FrontLoader_SetFiles(0);
	Front_DisplayScreen();

	GGame = 3;
	goto top;
}
