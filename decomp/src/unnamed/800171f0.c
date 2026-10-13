/* FUN_800171f0 (0x800171f0) */
#include "names.h"

typedef struct {
	int c[34];
} B136;

typedef struct {
	int p0, p1, p2, p3;
	char pad[0x20 - 0x10];
	int f20;
	int f24;
	char pad2[0x28 - 0x28];
} Z;

typedef struct {
	char *f00;
	char pad0[0x5c - 4];
	char *f5c;
	char *f60;
	char *f64;
	char pad1[4];
	char *f6c;
	char *f70;
	char *f74;
	char *f78;
	char *f7c;
	char *f80;
	char *f84;
	char pad2[0xc4 - 0x88];
	char *fc4;
	char *fc8;
	char pad3[0x124 - 0xcc];
	char *f124;
	char pad4[0x150 - 0x128];
	char *f150;
	char *f154;
	char *f158;
	char *f15c;
	char pad5[0x164 - 0x160];
	char *f164;
	char pad6[0x180 - 0x168];
	char pad7[4];
	char *f184;
	char *f188;
	char pad8[0x1b4 - 0x18c];
	char *f1b4;
	char *f1b8;
} S;

typedef struct {
	char pad[0xc];
	int fc;
	char pad2[0x78 - 0x10];
} T;

typedef struct {
	char pad[0x830];
	int f830;
	int f834;
} W;

extern B136 DAT_8009e31c;
extern B136 StartupGameState;
extern B136 LastGameState;
extern S GameText;
extern T Levels[];
extern W GCareer;
extern char soundBuf[];
extern char musicBuf[];
extern int GGame, GLevel;

int pYesNoMenu, pESMenu, pVSMenu, pVS2Menu, pVS3Menu, pEHMenu, pSoundMenu, soundVol, musicVol;

void (*DAT_800d1408)(void);
void FUN_80071fa4(void);
void CMenu_AddEntry(int, char *);
void CMenu_CentreY(int);
void CMenu_EntryOff(int, char *);
int CMenu_ctor(int, int, int, int, int, int, int);
void FUN_8008b900();
int CMenu_FindEntry(int, char *);
void CMenu_SetLine(int, int);
int __nw__6CClassUi(int);

void FUN_800171f0(void) {
	int q, q2, a1, a2, a3, a4, a5, a6, a7;

	DAT_800d1408 = FUN_80071fa4;
	StartupGameState = DAT_8009e31c;
	LastGameState = DAT_8009e31c;
	a1 = __nw__6CClassUi(384);

	if (a1)
		a1 = CMenu_ctor(a1, 256, 0, 0, 335, 256, 22);

	pYesNoMenu = a1;
	CMenu_AddEntry(a1, GameText.f1b4);
	CMenu_AddEntry(pYesNoMenu, GameText.f1b8);

	a2 = __nw__6CClassUi(384);

	if (a2)
		a2 = CMenu_ctor(a2, 256, 0, 0, 335, 256, 22);

	pESMenu = a2;
	CMenu_AddEntry(a2, GameText.f164);
	CMenu_AddEntry(pESMenu, (Levels[GLevel].fc && GGame == 1) ? GameText.f78 : GameText.f74);
	CMenu_AddEntry(pESMenu, GameText.f00);
	CMenu_AddEntry(pESMenu, GameText.f7c);
	CMenu_AddEntry(pESMenu, GameText.f80);
	CMenu_AddEntry(pESMenu, GameText.f84);
	CMenu_AddEntry(pESMenu, GameText.f70);
	CMenu_AddEntry(pESMenu, GameText.f188);

	if (GGame != 1) {
		CMenu_EntryOff(pESMenu, GameText.f70);

		if (GGame != 1)
			goto off0;
	}

	if (Levels[GLevel].fc) {
	off0:
		CMenu_EntryOff(pESMenu, GameText.f00);
	}

	if (GGame != 3)
		CMenu_EntryOff(pESMenu, GameText.f164);

	if (GGame == 2 || GGame == 7)
		CMenu_EntryOff(pESMenu, GameText.f84);

	CMenu_CentreY(pESMenu);
	CMenu_SetLine(
	    pESMenu,
	    CMenu_FindEntry(pESMenu, (Levels[GLevel].fc && GGame == 1) ? GameText.f78 : GameText.f74) &
	        255);

	a3 = __nw__6CClassUi(384);

	if (a3)
		a3 = CMenu_ctor(a3, 256, 0, 0, 335, 256, 22);

	pVSMenu = a3;
	CMenu_AddEntry(a3, GameText.f5c);
	CMenu_AddEntry(pVSMenu, GameText.f60);
	CMenu_AddEntry(pVSMenu, GameText.f64);
	CMenu_AddEntry(pVSMenu, GameText.f6c);
	CMenu_CentreY(pVSMenu);

	a4 = __nw__6CClassUi(384);

	if (a4)
		a4 = CMenu_ctor(a4, 256, 0, 0, 335, 256, 22);

	pVS2Menu = a4;
	CMenu_AddEntry(a4, GameText.f5c);
	CMenu_AddEntry(pVS2Menu, GameText.f60);
	CMenu_AddEntry(pVS2Menu, GameText.f6c);
	CMenu_CentreY(pVS2Menu);

	a5 = __nw__6CClassUi(384);

	if (a5)
		a5 = CMenu_ctor(a5, 256, 0, 0, 335, 256, 22);

	pVS3Menu = a5;
	CMenu_AddEntry(a5, GameText.f5c);
	CMenu_AddEntry(pVS3Menu, GameText.f60);
	CMenu_AddEntry(pVS3Menu, GameText.f64);
	CMenu_AddEntry(pVS3Menu, GameText.f124);
	CMenu_CentreY(pVS3Menu);

	a6 = __nw__6CClassUi(384);

	if (a6)
		a6 = CMenu_ctor(a6, 256, 0, 0, 335, 256, 22);

	pEHMenu = a6;
	CMenu_AddEntry(a6, GameText.fc4);
	CMenu_AddEntry(pEHMenu, GameText.f84);
	CMenu_AddEntry(pEHMenu, GameText.fc8);
	CMenu_AddEntry(pEHMenu, GameText.f188);
	CMenu_CentreY(pEHMenu);

	a7 = __nw__6CClassUi(384);

	if (a7)
		a7 = CMenu_ctor(a7, 256, 0, 0, 335, 256, 22);

	pSoundMenu = a7;
	q = (GCareer.f830 + 1) * 10 / 12;
	soundVol = q;
	FUN_8008b900(soundBuf, GameText.f150, q);
	CMenu_AddEntry(pSoundMenu, soundBuf);

	q2 = (GCareer.f834 + 1) * 10 / 12;
	musicVol = q2;
	FUN_8008b900(musicBuf, GameText.f154, q2);
	CMenu_AddEntry(pSoundMenu, musicBuf);
	CMenu_AddEntry(pSoundMenu, GameText.f158);
	CMenu_AddEntry(pSoundMenu, GameText.f15c);
	CMenu_CentreY(pSoundMenu);
}
