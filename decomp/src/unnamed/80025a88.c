/* FUN_80025a88 (0x80025a88) flags=-fstructure-noalias */
#include "names.h"
typedef struct L {
	char pad[12];
	int f;
	char q[104];
} L;

typedef struct SP {
	char *a;
	char *b;
	int pad;
	int c;
	char pad2[0x40 - 0x10];
	char *s;
	char pad3[0x154 - 0x44];
} SP;

typedef struct CB {
	char p0[308];
	int m308;
	char p1[404 - 312];
	int m404;
	char *m408;
	char p2[468 - 412];
	struct CB *m468;
	struct CC *m472;
	char p3[744 - 476];
	int m744;
	char p4[2160 - 748];
	int m2160;
} CB;

typedef struct CC {
	char p0[1412];
	int m1412;
} CC;

extern L Levels[];
extern SP Stats[];
extern int GVideoRestart[];
extern char GBoardStats[];
extern char DAT_800a8245[];
extern int SelectedSkater;

int GGame;
int SelectedSkater2;
int GNumberOfPlayers;
int GHorsePlayer;
CB *GSkater;
CB *DAT_800d17cc;

extern int GVideoReplay;
extern int GLevel;

void *__nw__5CItemUi(int);
CB *CBruce_ctor(void *);
CC *CCamera_ctor(void *, CB *, int);
void TrickTips_Init(void);
void CCamera_StartCompetitionIntro(CC *);
void CBruce_InitItem(CB *, char *);
void CBruce_GetStats(CB *, int);

void FUN_80025a88(void) {
	CB *pBruce;
	CC *pCam;
	CB *p2;
	CC *c2;

	pBruce = __nw__5CItemUi(3612);

	if (pBruce)
		pBruce = CBruce_ctor(pBruce);

	GSkater = pBruce;
	DAT_800d17cc = 0;
	pBruce->m2160 = 0;
	pCam = __nw__5CItemUi(1712);

	if (pCam)
		pCam = CCamera_ctor(pCam, pBruce, 0);

	pCam->m1412 = 1;
	TrickTips_Init();

	if (Levels[GLevel].f && GGame == 1 && GVideoRestart[0] == 0 &&
	    GVideoReplay == 0)
		CCamera_StartCompetitionIntro(pCam);

	if (GGame != 7 || GHorsePlayer == 0)
		pBruce->m744 = SelectedSkater;
	else
		pBruce->m744 = SelectedSkater2;

	pBruce->m472 = pCam;

	if (GNumberOfPlayers == 2 && GGame != 7)
		CBruce_InitItem(pBruce, Stats[pBruce->m744].b);
	else
		CBruce_InitItem(pBruce, Stats[pBruce->m744].a);

	if (*Stats[pBruce->m744].s == 'R' || *Stats[pBruce->m744].s == 'r')
		pBruce->m308 |= 2;

	pBruce->m408 = GBoardStats;
	CBruce_GetStats(pBruce, 0);

	if (Stats[pBruce->m744].c == 16384)
		pBruce->m404 |= 4096;

	if (GNumberOfPlayers == 2 && GGame != 7) {
		p2 = __nw__5CItemUi(3612);

		if (p2)
			p2 = CBruce_ctor(p2);

		p2->m2160 = 1;
		c2 = __nw__5CItemUi(1712);

		if (c2)
			c2 = CCamera_ctor(c2, p2, 0);

		c2->m1412 = 1;
		p2->m744 = SelectedSkater2;
		CBruce_InitItem(p2, Stats[SelectedSkater2].b);

		if (*Stats[p2->m744].s == 'R' || *Stats[p2->m744].s == 'r')
			p2->m308 |= 2;

		pBruce->m468 = p2;
		p2->m468 = pBruce;
		p2->m472 = c2;
		DAT_800d17cc = p2;
		p2->m408 = DAT_800a8245;
		CBruce_GetStats(p2, 1);
	}
}
