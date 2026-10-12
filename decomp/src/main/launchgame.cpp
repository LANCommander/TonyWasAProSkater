/* LaunchGame__Fv (0x80026e98) */
/* C++ (MAIN.CPP). Fuzzy demo twin (0.30; THPS1 drops the attract-mode wait loop); locals and the
   DUN label from its MAIN.SYM. Re-initialises the packs keeping the save game across
   Init_ForGame, loads the game, then (unless a restart was requested) primes the pads and plays
   levels, looping back to DUN while the game mode says to carry on. */
#include "game/card.h"
#include "game/pad.h"

enum EPackInitType { PACK_0, PACK_1 };

extern SSaveGame StartupGameState;
extern SControl Pad[2];
extern char s_ENDCODE_TWEAKJUMP[];
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

extern "C" {
	void FUN_80071f3c(void);
	void FUN_800171e8(const SPreferences *);
	void FUN_8005cba0(void);
	void FUN_8001ccf8(SSaveGame *);
	void PlayAway__Fv(void);
	int FUN_8008c190(const char *, ...);
	void Init_Cleanup__Fi(int);
	void FUN_80025a88(void);
	void Trig_ExecuteRestart__Fv(void);
	void FUN_800491e8(void);
	void FUN_8006b01c(int);
}

int LaunchGame() {
	int Result = 0;

	Pack_Init();
	Init_Restart(PACK_0, 16);

	{
		SSaveGame tmp = StartupGameState;

		FUN_80071f3c();

		StartupGameState = tmp;
	}

	FUN_800171e8(&StartupGameState.Preferences);

	if (!Result) {
		FUN_8005cba0();

		if (GAutoTest)
			DumpAutoTest();

		FUN_8001ccf8(&StartupGameState);
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
	Pad[0].X.Pressed = 1;
	Pad[0].X.Triggered = 0;
	Pad[1].X.Pressed = 1;
	Pad[1].X.Triggered = 0;
	CloseCam = 0;
	ViewportMode = 0;

	if (GNumberOfPlayers == 2 && GGame != 7)
		ViewportMode = 1;

	PlayAway__Fv();

	GClear1 = 0;
	GClear2 = 0;
	GHorseDoneStart = 0;

	if (EndCode == 7) {
		FUN_8008c190(s_ENDCODE_TWEAKJUMP);
		Init_Cleanup__Fi(1);
		Trig_ParseTRGFile();
		FUN_80025a88();
		Trig_ExecuteRestart__Fv();
		goto DUN;
	}

	FUN_8008c190(DAT_8009f5cc);
	Init_Restart(PACK_0, 16);
	FUN_800491e8();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	FUN_8006b01c(1);
	return 0;
}
