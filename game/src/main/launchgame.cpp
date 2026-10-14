/* LaunchGame__Fv (0x80026e98) */
/* C++ (MAIN.CPP). Fuzzy demo twin (0.30; THPS1 drops the attract-mode wait loop); locals and the
   DUN label from its MAIN.SYM. Re-initialises the packs keeping the save game across
   Init_ForGame, loads the game, then (unless a restart was requested) primes the pads and plays
   levels, looping back to DUN while the game mode says to carry on. */
#include "game/card.h"
#include "game/career.h"
#include "game/db.h"
#include "game/front.h"
#include "game/hires.h"
#include "game/init.h"
#include "game/main.h"
#include "game/pad.h"
#include "game/psyq.h"
#include "game/trig.h"

enum EPackInitType { PACK_0, PACK_1 };

extern SSaveGame StartupGameState;
extern SControl Pad[2];
extern char DAT_8009f5cc[];
extern int BackOut;
extern int DemoTimer;
extern int StartScreen;

int GAutoTest;
int GGame;
int GNumberOfPlayers;
int CloseCam;
int ViewportMode;
int EndCode;
int GClear1;
int GClear2;
int GHorseDoneStart;

void Pack_Init();
void Init_Restart(EPackInitType, int);
void DumpAutoTest();
void Horse_Reset();
void Trig_ParseTRGFile();

int LaunchGame() {
	int Result = 0;

	Pack_Init();
	Init_Restart(PACK_0, 16);

	{
		SSaveGame tmp = StartupGameState;

		Init_ForGame();

		StartupGameState = tmp;
	}

	Front_SetPreferences(StartupGameState.Preferences);

	if (!Result) {
		LoadingScreen_Main();

		if (GAutoTest)
			DumpAutoTest();

		Front_LoadGame(&StartupGameState);
	}

	if (BackOut) {
		if (DemoTimer)
			StartScreen = 1;
		else
			StartScreen = 4;

		BackOut = 0;
		return 0;
	}

	if (GGame == 7)
		Horse_Reset();
DUN:
	Pad[0].Ollie.Pressed = 1;
	Pad[0].Ollie.Triggered = 0;
	Pad[1].Ollie.Pressed = 1;
	Pad[1].Ollie.Triggered = 0;
	CloseCam = 0;
	ViewportMode = 0;

	if (GNumberOfPlayers == 2 && GGame != 7)
		ViewportMode = 1;

	PlayAway();

	GClear1 = 0;
	GClear2 = 0;
	GHorseDoneStart = 0;

	if (EndCode == 7) {
		printf("ENDCODE_TWEAKJUMP\n");
		Init_Cleanup(1);
		Trig_ParseTRGFile();
		CreateSkatersAndCameras();
		Trig_ExecuteRestart();
		goto DUN;
	}

	printf(DAT_8009f5cc);
	Init_Restart(PACK_0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	HiRes_Init(1);
	return 0;
}
