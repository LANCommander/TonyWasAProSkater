/* Competition_WinLoseLogic__FR13EPausedStatusRi (0x80075560) flags=-fstructure-noalias */
#include "names.h"

/* Competition win/lose logic (COMPETITION.CPP, demo Competition_WinLoseLogic), as C; sibling of
   FUN_80076880 and written the same way. */
struct VT {
	char pad[8];
	short off;
	short p;
	void (*fn)(char *, int);
};

typedef struct O {
	char p[0x48];
	struct VT *vt;
	char q[0x160 - 0x4c];
	unsigned f160;
} O;

struct G {
	char p[0xc8];
	O *a;
	O *b;
	O *c;
	char q[0x12c - 0xd4];
	int f12c;
};

extern struct G theCompetition;
extern int GameText_1b4[];
extern int pYesNoMenu;
extern int GNumberOfPlayers;
extern int GLevel;

struct GA {
	char pad[0x984];
	int arr[4];
};

extern struct GA GCareer;

struct L {
	char pad[0x50];
	int x[1];
	char pad2[0x78 - 0x54];
};

extern struct L Levels[];

struct H {
	char p[0x80a8];
	int a;
	char q[0x10154 - 0x80ac];
	int b;
};

extern struct H DoubleBuffer;
extern char Pad[];

struct W {
	char p[0x2e8];
	int i;
};

extern struct W *GSkater;

void Crowd_Stop(void);
void Flash_Reset(void);
int Flash_FadeFinished(void);
void GameFMV_PlayMovie(int);
int Mem_NewX(int, int, int);
void Mem_Delete(int);
int FUN_80073f9c(void);
int FUN_80077124(void *);
void CdControlB(int, int, int);
int CMenu_FindEntry(int, int);
void Flash_Fade(int, int);
void Flash_Update(void);
void GameFMV_StopFMV(void);
void Mess_DeleteAll(void);
void Pad_ActuatorOff(int, int);
void Pad_ClearAll(void);
void Pad_ClearTriggers(void *);
void Pause__Fi(int);
void CMenu_SetLine(int, int);

#define DEL(o)                                                                                     \
	if (o)                                                                                         \
	(o)->vt->fn((char *)(o) + (o)->vt->off, 3)

void Competition_WinLoseLogic(int *a0, int *a1) {
	int v, e;
	struct W *gg;
	struct G *g = &theCompetition;

	if (g->f12c) {
		Flash_Update();

		if (Flash_FadeFinished()) {
			g->f12c = 0;
			CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText_1b4[0]) & 0xff);
			DEL(g->a);
			DEL(g->b);
			DEL(g->c);

			gg = GSkater;
			e = GLevel;
			g->a = 0;
			g->b = 0;
			g->c = 0;
			GCareer.arr[gg->i] |= 1 << e;
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
			GameFMV_PlayMovie(-11);
			Flash_Reset();

			DoubleBuffer.a = Mem_NewX((Levels[GLevel].x[GNumberOfPlayers - 1] + 0x17c00) * 2, 1, 1);
			DoubleBuffer.b = DoubleBuffer.a + 0x17c00 + Levels[GLevel].x[GNumberOfPlayers - 1];
			*a0 = 27;
		}
	} else {
		Crowd_Stop();

		if (*a1) {
			if (FUN_80077124(g)) {
				v = 1;

				if (g->a) {
					int c = g->a->f160 < 40;

					v = c == 0;
				}

				if (v) {
					if (FUN_80073f9c()) {
						g->f12c = 1;
						Flash_Fade(32, 0);
						return;
					}

					*a0 = 7;
					CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText_1b4[0]) & 0xff);
				} else
					return;
			} else {
				*a0 = 14;
			}

			DEL(g->a);
			DEL(g->b);
			DEL(g->c);

			g->a = 0;
			g->b = 0;
			g->c = 0;
			GCareer.arr[GSkater->i] |= 1 << GLevel;
		}
	}
}
