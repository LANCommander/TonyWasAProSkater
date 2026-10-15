/* CreateSkatersAndCameras__Fv (0x80025a88) flags=-fstructure-noalias */
#include "game/bruce.h"
#include "game/camera.h"
#include "game/career.h"

extern SLevel Levels[];
extern SStats Stats[];
extern int GVideoRestart[];
extern uchar GBoardStats[];
extern uchar DAT_800a8245[];
extern int SelectedSkater;

int GGame;
int SelectedSkater2;
int GNumberOfPlayers;
int GHorsePlayer;
CBruce *GSkater;
CBruce *DAT_800d17cc;

extern int GVideoReplay;
extern int GLevel;

void TrickTips_Init();

void CreateSkatersAndCameras(void) {
	CBruce *pBruce;
	CCamera *pCam;
	CBruce *p2;
	CCamera *c2;

	pBruce = new CBruce();

	GSkater = pBruce;
	DAT_800d17cc = 0;
	pBruce->mPadIndex = 0;
	pCam = new CCamera(pBruce, 0);

	pCam->mCameraMode = (ECameraMode)1;
	TrickTips_Init();

	if (Levels[GLevel].isCompetition && GGame == 1 && GVideoRestart[0] == 0 && GVideoReplay == 0)
		pCam->StartCompetitionIntro();

	if (GGame != 7 || GHorsePlayer == 0)
		pBruce->mSkaterIndex = SelectedSkater;
	else
		pBruce->mSkaterIndex = SelectedSkater2;

	pBruce->mpCamera = pCam;

	if (GNumberOfPlayers == 2 && GGame != 7)
		pBruce->InitItem(Stats[pBruce->mSkaterIndex].PSXName2);
	else
		pBruce->InitItem(Stats[pBruce->mSkaterIndex].PSXName);

	/* stance: Regular */
	if (*Stats[pBruce->mSkaterIndex].pBio[4] == 'R' || *Stats[pBruce->mSkaterIndex].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;

	pBruce->mpStatBonus = GBoardStats;
	pBruce->GetStats(0);

	if (Stats[pBruce->mSkaterIndex].ComboID == 16384)
		pBruce->mSubObjectDisplayMask |= 4096;

	if (GNumberOfPlayers == 2 && GGame != 7) {
		p2 = new CBruce();

		p2->mPadIndex = 1;
		c2 = new CCamera(p2, 0);

		c2->mCameraMode = (ECameraMode)1;
		p2->mSkaterIndex = SelectedSkater2;
		p2->InitItem(Stats[SelectedSkater2].PSXName2);

		if (*Stats[p2->mSkaterIndex].pBio[4] == 'R' || *Stats[p2->mSkaterIndex].pBio[4] == 'r')
			p2->mExtraFlags |= 2;

		pBruce->mpOtherSkater = p2;
		p2->mpOtherSkater = pBruce;
		p2->mpCamera = c2;
		DAT_800d17cc = p2;
		p2->mpStatBonus = DAT_800a8245;
		p2->GetStats(1);
	}
}
