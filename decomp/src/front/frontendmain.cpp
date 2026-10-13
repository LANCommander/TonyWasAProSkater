/* FUN_8005bb70 (0x8005bb70) */
/* C++ (front end). The front-end screen dispatcher, called once from FUN_8006b3f8 (after the boot
   init) and never returning. No demo twin (THPS2's FrontEnd2_Main is a different, table-driven
   design). Each pass:
     - (re)start: on a soft reset (START+SELECT, DAT_800d1cb8) unload everything and clear it; set
       DemoName to "demoA.rec" and clear the video-restart data.
     - top: in autotest mode (GAutoTest) cycle level/skater/players and launch straight away.
       Otherwise, coming back from a game (StartScreen = 1..6) jump straight back into the screen
       the player left from; else run the title screen (FUN_80060e98) and then the main menu
       (FUN_8005de7c).
     - main-menu choice: 3/4/17/18/19 = 1-player game modes -> skater select (FUN_80059794);
       33/34/35/1234567 = 2-player modes -> 2P skater select (FUN_80056754); 1553/1554 = replay
       the 1P/2P video restart; 99, 1569-1574, 25202-25207 = FMV movies; 100/101 = credits.
     - 1P: skater select -> board select (FUN_80054510); 2P: skater select -> board select
       (FUN_80051660); both then level select (FUN_8006658c). -1 from a screen backs up one.
     - launch: record the choice, start the game (FUN_8006b434 -> LaunchGame), tear down, reload
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
	extern unsigned char DAT_800a7068[];  /* last 1P skater */
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
	extern int GVideoReplay;             /* GVideoReplay */
	extern int GLevel;                   /* GLevel */
	extern SName s_demoA_rec;            /* "demoA.rec" */
	extern char s_mainf_fnt_800a1938[];  /* s_mainf_fnt_800a1938 */
	extern char s_Start_1_fnt[];         /* s_Start_1_fnt */
	extern char s_stats2_fnt_800a1950[]; /* s_stats2_fnt_800a1950 */
	extern char s_shell_vab[];           /* s_shell_vab */

	int StartScreen; /* screen to return to */
	short DAT_800d1ca0;
	int DemoTimer; /* DemoTimer */
	int DAT_800d1cb4;
	int DAT_800d1cb8; /* soft reset */

	void Career_Init__Fv(void);
	void Options_SetPreferences__Fv(void);
	short FUN_80014ba4(SName *);
	void FUN_800161d8(void);
	void FUN_800171f0(void);
	void Mess_SetTextJustify__FUc(int);
	void FUN_8001f0dc(void);
	void FUN_8001f1b4(int);
	void GameFMV_PlayMovie__Fi(int);
	void FUN_800491e8(void);
	void FUN_8004b09c(char *);
	void Mem_Delete__FPv(void *);
	void FUN_8004fe28(int, int);
	int FUN_80051660(int);
	int FUN_80054510(int);
	int FUN_80056754(void);
	int FUN_80059794(void);
	void FUN_8005c678(void);
	void FUN_8005c734(int);
	void FUN_8005c85c(void);
	void FUN_8005c8ac(void);
	void FUN_8005c8dc(int);
	void FUN_8005ca10(void);
	void FUN_8005ca88(void);
	void FUN_8005cae8(void);
	void FUN_8005d35c(int, int, int);
	int FUN_8005de7c(int);
	void FUN_80060e98(void);
	void FUN_800642d4(int);
	int FUN_8006658c(int);
	void FUN_8006af64(void);
	void FUN_8006b01c(int);
	void FUN_8006b434(int, int);
	void FUN_8005bb70(void);
}

static inline int GetReturnScreen() {
	int s = StartScreen;

	StartScreen = 5;
	return s;
}

void FUN_8005bb70(void) {
	int choice;
	int players;
	int mode;
	int skater;
	int menu;
	int r;

	FUN_8005c678();
restart:
	if (DAT_800d1cb8) {
	reset:
		FUN_8001f0dc();
		FUN_8005ca10();
		FUN_8005cae8();
		FUN_8005c678();
	}

	DemoName = s_demoA_rec;
	DAT_800d1cb4 = 1;
	Front_ClearGVideoRestartData();
	Career_Init__Fv();
	FUN_800161d8();
top:
	FUN_8005c8ac();

	DAT_800d1dc0 = 1;
	GNumberOfPlayers = 1;
	DemoTimer = 1600;
	FUN_800642d4(2);

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

	FUN_8005ca88();

	if (StartScreen) {
		FUN_8005c734(157);
		Mess_LoadFont(s_mainf_fnt_800a1938, -1, -1, -1);
		Mess_SetRGB(128, 128, 128, 0);
		Mess_SetFontScale(256);
		Mess_SetTextJustify__FUc(1);
		FUN_8001f1b4(4);

		mode = 0;
		skater = DAT_800a7068[0];
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

				FUN_800642d4(1);
				goto board1;
			case 1:
				if (players == 1)
					FUN_800642d4(1);

				goto level;
			case 3:
				GVideoRestart.NumSkaters = 0;
				menu = 96;
				FUN_8005c8dc(0);
				goto mainmenu;
			case 4:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FUN_8005c8dc(0);
				goto mainmenu;
			case 5:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FUN_8005c8dc(0);
				goto mainmenu;
			case 6:
				if (players == 1)
					FUN_800642d4(1);

				choice = GLevel;
				Mess_SetCurrentFont(s_mainf_fnt_800a1938);
				Mess_UnloadFont();
				FUN_8005cae8();
				goto launch;
		}
	}

	StartScreen = 5;
title:
	FUN_8005ca10();
	FUN_80060e98();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont(s_Start_1_fnt);
	Mess_UnloadFont();

	if (!DemoTimer)
		goto demo;

	menu = 0;
mainmenu:
	FUN_8005c734(157);
	Pack_DefragmentFully();

	choice = FUN_8005de7c(menu);

	if (DAT_800d1cb8)
		goto reset;

	if (!DemoTimer)
		goto demo;

	Options_SetPreferences__Fv();

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
				Career_Init__Fv();
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
			FUN_8005ca10();
			FUN_8005cae8();
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
	FUN_800642d4(1);

	GNumberOfPlayers = 1;
	skater = FUN_80059794();

	if (DAT_800d1cb8)
		goto reset;

	if (skater == -1) {
		FUN_800642d4(2);
		goto mainmenu;
	}

	goto board1;
skater2:
	Pack_DefragmentFully();

	skater = FUN_80056754();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont(s_stats2_fnt_800a1950);
	Mess_UnloadFont();
	Mess_SetCurrentFont(s_mainf_fnt_800a1938);

	if (skater == -1)
		goto mainmenu;

	goto board2;
board1:
	r = FUN_80054510(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater1;

	goto level;
board2:
	r = FUN_80051660(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater2;
level:
	choice = FUN_8006658c(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (choice == -1) {
		if (players == 1)
			goto board1;

		if (players == 2)
			goto board2;
	}

	Mess_SetCurrentFont(s_mainf_fnt_800a1938);
	Mess_UnloadFont();
	FUN_8005cae8();
launch:
	FUN_8001f0dc();
	FUN_8005ca10();
	FUN_8005d35c(choice, mode, skater);

	GNumberOfPlayers = players;
	DAT_800d1dc0 = 0;

	if (!GVideoReplay && Levels[choice].isCompetition == 1)
		Competition_Init();

	FUN_8005c85c();
	FUN_8006b434(choice, skater);
	Init_Restart(PACK_0, 16);
	FUN_800491e8();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	FUN_8006b01c(0);
	FUN_800161d8();
	FUN_8004b09c(s_shell_vab);
	FUN_800171f0();

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
		FUN_80014ba4(&DemoName);
	} while (0);

	mode = 2;
	menu = 16;
	players = GVideoRestart.NumSkaters;
	skater = GVideoRestart.Skater;
	choice = GVideoRestart.Level;
	SelectedSkater2 = GVideoRestart.Skater2;
	GGame = GVideoRestart.GGame;
	FUN_8005cae8();

	StartScreen = 4;
	goto launch;

movie:
	FUN_8006af64();

	DAT_800d1dc0 = 0;
	FUN_8005c85c();

	if ((unsigned)(choice - 99) < 2)
		menu = 96;
	else if ((unsigned)(choice - 1569) < 6)
		menu = 1568;
	else if ((unsigned)(choice - 25202) < 6)
		menu = 25200;

	switch (choice) {
		case 99:
			GameFMV_PlayMovie__Fi(-26);
			break;
		case 1569:
			GameFMV_PlayMovie__Fi(-25);
			break;
		case 1570:
			GameFMV_PlayMovie__Fi(-12);
			break;
		case 1571:
			GameFMV_PlayMovie__Fi(-13);
			break;
		case 1572:
			GameFMV_PlayMovie__Fi(-14);
			break;
		case 1573:
			GameFMV_PlayMovie__Fi(-15);
			break;
		case 1574:
			GameFMV_PlayMovie__Fi(-16);
			break;
		case 25202:
			GameFMV_PlayMovie__Fi(-17);
			break;
		case 25203:
			GameFMV_PlayMovie__Fi(-18);
			break;
		case 25204:
			GameFMV_PlayMovie__Fi(-19);
			break;
		case 25205:
			GameFMV_PlayMovie__Fi(-20);
			break;
		case 25206:
			GameFMV_PlayMovie__Fi(-21);
			break;
		case 25207:
			GameFMV_PlayMovie__Fi(-11);
			break;
		case 100:
			break;
	}

	if (DoubleBuffer[0].Polys)
		Mem_Delete__FPv(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	FUN_8006b01c(1);
	FUN_8005c8dc(0);
	FUN_800161d8();
	goto mainmenu;

credits:
	FUN_8005c734(choice == 101 ? 157 : -1);
	FUN_8004fe28(choice != 100, 1);

	if (DoubleBuffer[0].Polys)
		Mem_Delete__FPv(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	FUN_8006b01c(1);
	FUN_8005c8dc(0);
	FUN_800161d8();

	GGame = 3;
	goto top;
}
