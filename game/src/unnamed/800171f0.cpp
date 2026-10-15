/* Front_Init__Fv (0x800171f0) */
#include "game/front.h"
#include "game/career.h"
#include "game/psyq.h"

typedef struct {
	int c[34];
} B136;

typedef struct {
	char pad[0x830];
	int f830;
	int f834;
} W;

extern B136 DAT_8009e31c;
extern B136 StartupGameState;
extern B136 LastGameState;
extern LocalText GameText;
extern SLevel Levels[];
extern W GCareer;
extern char soundBuf[];
extern char musicBuf[];
extern int GGame, GLevel;

extern CMenu *pYesNoMenu, *pESMenu, *pVSMenu, *pVS2Menu, *pVS3Menu, *pEHMenu, *pSoundMenu;
extern int soundVol, musicVol;

extern void (*DAT_800d1408)(void);
extern "C" void FUN_80071fa4(void);

void Front_Init(void) {
	int q, q2;
	CMenu *a1, *a2, *a3, *a4, *a5, *a6, *a7;

	DAT_800d1408 = FUN_80071fa4;
	StartupGameState = DAT_8009e31c;
	LastGameState = DAT_8009e31c;
	a1 = new CMenu(256, 0, 0, 335, 256, 22);

	pYesNoMenu = a1;
	a1->AddEntry(GameText.pYes);
	pYesNoMenu->AddEntry(GameText.pNo);

	a2 = new CMenu(256, 0, 0, 335, 256, 22);

	pESMenu = a2;
	a2->AddEntry(GameText.pUpdateSaveFile);
	pESMenu->AddEntry((Levels[GLevel].isCompetition && GGame == 1) ? GameText.pRestart : GameText.pRetry);
	pESMenu->AddEntry(GameText.pLevelHints);
	pESMenu->AddEntry(GameText.pSelectLevel);
	pESMenu->AddEntry(GameText.pEquipmentSelect);
	pESMenu->AddEntry(GameText.pWatchReplay);
	pESMenu->AddEntry(GameText.pSaveCareer);
	pESMenu->AddEntry(GameText.pQuit);

	if (GGame != 1) {
		pESMenu->EntryOff(GameText.pSaveCareer);

		if (GGame != 1)
			goto off0;
	}

	if (Levels[GLevel].isCompetition) {
	off0:
		pESMenu->EntryOff(GameText.pLevelHints);
	}

	if (GGame != 3)
		pESMenu->EntryOff(GameText.pUpdateSaveFile);

	if (GGame == 2 || GGame == 7)
		pESMenu->EntryOff(GameText.pWatchReplay);

	pESMenu->CentreY();
	pESMenu->SetLine(
	    pESMenu->FindEntry((Levels[GLevel].isCompetition && GGame == 1) ? GameText.pRestart
	                                                                     : GameText.pRetry) &
	    255);

	a3 = new CMenu(256, 0, 0, 335, 256, 22);

	pVSMenu = a3;
	a3->AddEntry(GameText.pFullReplay);
	pVSMenu->AddEntry(GameText.pHighlights);
	pVSMenu->AddEntry(GameText.pSaveVideo);
	pVSMenu->AddEntry(GameText.pExit);
	pVSMenu->CentreY();

	a4 = new CMenu(256, 0, 0, 335, 256, 22);

	pVS2Menu = a4;
	a4->AddEntry(GameText.pFullReplay);
	pVS2Menu->AddEntry(GameText.pHighlights);
	pVS2Menu->AddEntry(GameText.pExit);
	pVS2Menu->CentreY();

	a5 = new CMenu(256, 0, 0, 335, 256, 22);

	pVS3Menu = a5;
	a5->AddEntry(GameText.pFullReplay);
	pVS3Menu->AddEntry(GameText.pHighlights);
	pVS3Menu->AddEntry(GameText.pSaveVideo);
	pVS3Menu->AddEntry(GameText.pReturnToCompetition);
	pVS3Menu->CentreY();

	a6 = new CMenu(256, 0, 0, 335, 256, 22);

	pEHMenu = a6;
	a6->AddEntry(GameText.pNextHeat);
	pEHMenu->AddEntry(GameText.pWatchReplay);
	pEHMenu->AddEntry(GameText.pEndComp);
	pEHMenu->AddEntry(GameText.pQuit);
	pEHMenu->CentreY();

	a7 = new CMenu(256, 0, 0, 335, 256, 22);

	pSoundMenu = a7;
	q = (GCareer.f830 + 1) * 10 / 12;
	soundVol = q;
	sprintf(soundBuf, GameText.pSFXLevel, q);
	pSoundMenu->AddEntry(soundBuf);

	q2 = (GCareer.f834 + 1) * 10 / 12;
	musicVol = q2;
	sprintf(musicBuf, GameText.pMusicLevel, q2);
	pSoundMenu->AddEntry(musicBuf);
	pSoundMenu->AddEntry(GameText.pSkipTrack);
	pSoundMenu->AddEntry(GameText.pBackToMainMenu);
	pSoundMenu->CentreY();
}
