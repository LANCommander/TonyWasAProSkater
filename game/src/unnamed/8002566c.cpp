/* PlayAway_Init__Fv (0x8002566c) */
#include "game/career.h"
#include "game/psyq.h"

/* PlayAway_Init (demo twin PlayAway_Init). LastDeck[] picks each player's board texture,
   LastWheel[] the W1*.bmp colour; reading LastWheel through GCareer (not as a separate global)
   fixes the loop's preheader order (sp+16 hoisted before sp+48). */
extern SStats Stats[];
extern SCareer GCareer;
extern short ApocalypseViewport_c[], LeftViewport_c[], RightViewport_c[], TopViewport_c[],
    BottomViewport_c[], TopLeftViewport_c[], BottomRightViewport_c[], TopLeftBigViewport_c[],
    BottomRightBigViewport_c[];
extern int SelectedSkater;
extern char InShell;
extern int DAT_800d1f14, DAT_800d1f18;

int SelectedSkater2, EndCode, TTime, EveryOther, XFrames, GNumberOfPlayers, GState, GStartTime,
    GSkaterRegion, DAT_800d1798, GClear1, GClear2, XFramesShifted, DAT_800d183c, DAT_800d1840,
    LastXblanks;

extern volatile int Vblanks, Xblanks;

extern "C" void FUN_8007ef30(void);
int Mess_LoadFont(char *, int, int, int);
void Pad_ClearAll();
void Redbook_InitialRand(int);
void ReplaceTexture(uint, char *);

void PlayAway_Init() {
	char nm[2][16];
	int i, j, k;

	GClear1 = 0;
	GClear2 = 0;
	strcpy(nm[0], Stats[SelectedSkater].Boards[(uchar)GCareer.LastDeck[0]].File);
	strcpy(nm[1], Stats[SelectedSkater2].Boards[(uchar)GCareer.LastDeck[1]].File);

	for (i = 0; i < 2; i++)
		nm[i][1] = 95;

	ReplaceTexture(Stats[SelectedSkater].BoardChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].BoardChecksum, nm[1]);

	{
		char *tab[15] = { "W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp",
		                  "W1brow01.bmp", "W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp",
		                  "W1gree03.bmp", "W1oran01.bmp", "W1purp01.bmp", "W1red_01.bmp",
		                  "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp" };

		for (j = 0; j < 2; j++) {
			strcpy(nm[j], tab[(uchar)GCareer.LastWheel[j]]);

			nm[j][1] = 95;
		}
	}

	ReplaceTexture(Stats[SelectedSkater].WheelChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].WheelChecksum, nm[1]);

	for (k = 0; k < 2; k++)
		nm[k][1] = 84;

	ReplaceTexture(Stats[SelectedSkater].TruckChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].TruckChecksum, nm[1]);

	ApocalypseViewport_c[0] = 3413;
	LeftViewport_c[0] = 2275;
	RightViewport_c[0] = 2275;
	TopViewport_c[0] = 3413;
	BottomViewport_c[0] = 3413;
	TopLeftViewport_c[0] = 2275;
	BottomRightViewport_c[0] = 2275;
	TopLeftBigViewport_c[0] = 2275;
	GState = 0;
	BottomRightBigViewport_c[0] = 2275;
	Vblanks = 0;
	Xblanks = 0;
	XFramesShifted = 0;
	XFrames = 0;
	TTime = 0;
	EveryOther = 0;
	InShell = 0;
	LastXblanks = Xblanks;
	Pad_ClearAll();

	DAT_800d1f14 = 0;
	DAT_800d1f18 = 0;
	DAT_800d183c = 0;
	DAT_800d1840 = 0;
	EndCode = 0;
	Redbook_InitialRand(0x12a6cc58);
	FUN_8007ef30();

	GSkaterRegion = Mess_LoadFont("time.fnt", -1, -1, 0);
	DAT_800d1798 = Mess_LoadFont("score.fnt", -1, -1, 0);
	GStartTime = Mess_LoadFont("ingame1.fnt", 2, 2, -1);
}
