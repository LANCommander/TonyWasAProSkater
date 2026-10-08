/* FUN_8002566c (0x8002566c) */
#include "names.h"
/* PlayAway_Init (demo twin PlayAway_Init). b826/b827 pick each player's skater, b828[] the W1*.bmp
   colour; reading b828 through GCareer (not as a separate global) fixes the loop's
   preheader order (sp+16 hoisted before sp+48). */
struct E {
	char p[0x48];
	int f48, f4c, f50;
	char *n[64];
};

extern struct E Stats[];

struct GA {
	char pad[0x826];
	unsigned char b826, b827, b828[2];
};

extern struct GA GCareer;
extern short ApocalypseViewport_c[], LeftViewport_c[], RightViewport_c[], TopViewport_c[], BottomViewport_c[],
    TopLeftViewport_c[], BottomRightViewport_c[], TopLeftBigViewport_c[], BottomRightBigViewport_c[];
extern int SelectedSkater;
extern char InShell;
extern int DAT_800d1f14, DAT_800d1f18;

int SelectedSkater2, EndCode, TTime, EveryOther, XFrames, GNumberOfPlayers,
    GState, GStartTime, GSkaterRegion, DAT_800d1798, GClear1, GClear2,
    XFramesShifted, DAT_800d183c, DAT_800d1840, LastXblanks;

extern volatile int Vblanks, Xblanks;

void FUN_8007ef30(void);
char *strcpy(char *, const char *);
int Mess_LoadFont__FPciii(char *, int, int, int);
void Pad_ClearAll(void);
void Redbook_InitialRand(int);
void ReplaceTexture(int, char *);

void FUN_8002566c(void) {
	char nm[2][16];
	int i, j, k;

	GClear1 = 0;
	GClear2 = 0;
	strcpy(nm[0], Stats[SelectedSkater].n[GCareer.b826 << 3]);
	strcpy(nm[1], Stats[SelectedSkater2].n[GCareer.b827 << 3]);

	for (i = 0; i < 2; i++)
		nm[i][1] = 95;

	ReplaceTexture(Stats[SelectedSkater].f48, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].f48, nm[1]);

	{
		char *tab[15] = { "W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp",
		                  "W1brow01.bmp", "W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp",
		                  "W1gree03.bmp", "W1oran01.bmp", "W1purp01.bmp", "W1red_01.bmp",
		                  "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp" };

		for (j = 0; j < 2; j++) {
			strcpy(nm[j], tab[GCareer.b828[j]]);

			nm[j][1] = 95;
		}
	}

	ReplaceTexture(Stats[SelectedSkater].f4c, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].f4c, nm[1]);

	for (k = 0; k < 2; k++)
		nm[k][1] = 84;

	ReplaceTexture(Stats[SelectedSkater].f50, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].f50, nm[1]);

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

	GSkaterRegion = Mess_LoadFont__FPciii("time.fnt", -1, -1, 0);
	DAT_800d1798 = Mess_LoadFont__FPciii("score.fnt", -1, -1, 0);
	GStartTime = Mess_LoadFont__FPciii("ingame1.fnt", 2, 2, -1);
}
