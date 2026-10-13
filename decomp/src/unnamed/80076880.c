/* Competition_EndGameLogic__FR13EPausedStatusRi (0x80076880) flags=-fstructure-noalias */
#include "names.h"

/* Competition end-of-game logic (COMPETITION.CPP, demo Competition_EndGameLogic), as C: after the
   results flash, kills the medal objects, records the medal, and reloads/plays the ending; else
   waits for the prompt and offers the continue menu. */
struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	char pad[72];
	struct VT *vt;
	char pad2[0x160 - 0x4c];
	unsigned u160;
};

struct J {
	char pad[0xc8];
	struct O *o1;
	struct O *o2;
	struct O *o3;
	char pad2[0x12c - 0xd4];
	int f;
};

struct S {
	char pad[12];
	int f;
	char q[104];
};

extern struct J theCompetition;

struct G {
	char pad[0x984];
	int arr[4];
};

struct B {
	char pad[0x80a8];
	int a;
	char pad2[0x10154 - 0x80ac];
	int b;
};

struct L {
	char pad[0x50];
	int x[1];
	char pad2[0x78 - 0x54];
};

extern struct G GCareer;
extern struct B DoubleBuffer;
extern struct L Levels[];
extern char Pad[];
extern char *GSkater;
extern int GLevel;
extern int GNumberOfPlayers;
extern int GRollCredits;
extern char *pYesNoMenu;
extern char *GameText_1b4[];

void Flash_Update(void);
int Flash_FadeFinished(void);
void GameFMV_StopFMV(void);
void Pad_ClearAll(void);
void Pad_ClearTriggers(char *);
void Pad_ActuatorOff(int, int);
void Mess_DeleteAll(void);
void CdControlB(int, int, int);
void Pause__Fi(int);
void Mem_Delete(int);
void GameFMV_PlayMovie(int);
void Flash_Reset(void);
int Mem_NewX(int, int, int);
int CMenu_FindEntry(char *, char *);
void CMenu_SetLine(char *, unsigned char);
int Competition_WonGameAllGold(void);
void Flash_Fade(int, int);

void Competition_EndGameLogic(int *a, int *b) {
	char *g;
	int e;

	if (theCompetition.f) {
		Flash_Update();

		if (Flash_FadeFinished()) {
			theCompetition.f = 0;

			if (theCompetition.o1)
				theCompetition.o1->vt->fn((char *)theCompetition.o1 + theCompetition.o1->vt->off,
				                          3);

			if (theCompetition.o2)
				theCompetition.o2->vt->fn((char *)theCompetition.o2 + theCompetition.o2->vt->off,
				                          3);

			if (theCompetition.o3)
				theCompetition.o3->vt->fn((char *)theCompetition.o3 + theCompetition.o3->vt->off,
				                          3);

			e = GLevel;
			g = GSkater;
			theCompetition.o1 = 0;
			theCompetition.o2 = 0;
			theCompetition.o3 = 0;
			GCareer.arr[*(int *)(g + 744)] |= 1 << e;
			GameFMV_StopFMV();
			Pad_ClearAll();
			Pad_ClearTriggers(Pad);
			Pad_ClearTriggers(Pad + 364);
			Pad_ActuatorOff(0, 0);
			Pad_ActuatorOff(0, 1);
			Mess_DeleteAll();
			CdControlB(8, 0, 0);
			Pause__Fi(10);
			Mem_Delete(DoubleBuffer.a);
			GameFMV_PlayMovie(-(*(int *)(GSkater + 744) + 12));
			Flash_Reset();

			DoubleBuffer.a = Mem_NewX((Levels[GLevel].x[GNumberOfPlayers - 1] + 0x17c00) * 2, 1, 1);
			DoubleBuffer.b = DoubleBuffer.a + 0x17c00 + Levels[GLevel].x[GNumberOfPlayers - 1];
			*a = 7;
		}
	} else if (*b) {
		int v = 1;

		if (theCompetition.o1) {
			int c = theCompetition.o1->u160 < 40;

			v = c == 0;
		}

		if (v) {
			GRollCredits = 1;
			CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText_1b4[0]));

			if (Competition_WonGameAllGold()) {
				theCompetition.f = 1;
				Flash_Fade(32, 0);
				return;
			}

			*a = 7;

			if (theCompetition.o1)
				theCompetition.o1->vt->fn((char *)theCompetition.o1 + theCompetition.o1->vt->off,
				                          3);

			if (theCompetition.o2)
				theCompetition.o2->vt->fn((char *)theCompetition.o2 + theCompetition.o2->vt->off,
				                          3);

			if (theCompetition.o3)
				theCompetition.o3->vt->fn((char *)theCompetition.o3 + theCompetition.o3->vt->off,
				                          3);

			theCompetition.o1 = 0;
			theCompetition.o2 = 0;
			theCompetition.o3 = 0;
			GCareer.arr[*(int *)(GSkater + 744)] |= 1 << GLevel;
		}
	}
}
