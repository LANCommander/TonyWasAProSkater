/* Front_SwitchOnEndLevelMenu__Fv (0x8001a2bc) */
/* C++ (FRONT.CPP). Fuzzy demo twin (0.37; THPS1 drops the checksum debug print); locals from its
   MAIN.SYM. Leaving a level: stop any replay video, pick the end-of-level menu, enter the run's
   score in the level's high-score table (shifting the lower entries down), then set up the menu. */
#include "game/bruce.h"
#include "game/video.h"
#include "game/career.h"
#include "game/mess.h"

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern SLevel Levels[];
extern SCareer GCareer;
extern FallingTextEffect DAT_800a5b28[];
extern CBruce *MechList;
extern CBruce *GSkater;
extern int GState;
extern int GNumberOfPlayers;
extern int GGame;
extern int GLevel;
extern int GHorseWaitTime;

int Status;
int GHighScoreLineIndex;
int GHighScoreInitialCursorPos;
int GHorseScale;
int GHorseContinue;

void Video_Stop(CBruce *);
void InitStatsSwisch();
int SFX_PlayX(unsigned int, short, int);

extern "C" {
	void CommonSwitchOnMenuCode__Fv(void);
	int FUN_8008b900(char *, const char *, ...);
}

void Front_SwitchOnEndLevelMenu() {
	CBruce *pSkater;
	CBruce *pSkater2;

	GState = 0;

	if (Status == 11 || Status == 13)
		return;

	pSkater = MechList;
	pSkater2 = (CBruce *)pSkater->mNextItem;

	if (GVInfo1.VideoMode)
		Video_Stop(pSkater);

	if (pSkater2 && GVInfo2.VideoMode)
		Video_Stop(pSkater2);

	if (GSkater->mNumTapes && GNumberOfPlayers == 1) {
		SFX_PlayX(19, 8192, 0);

		Status = 11;
	} else if (GGame == 2) {
		Status = 14;
	} else {
		Status = 13;
	}

	if ((GGame == 1 && !Levels[GLevel].isCompetition) || GGame == 3) {
		int i;

		GHighScoreLineIndex = -1;
		GHighScoreInitialCursorPos = 0;

		for (i = 0; i < 8; i++) {
			if (pSkater->mPanel.mTotalScore >= GCareer.HighScore[GLevel][i].Score) {
				int j;

				for (j = 7; j > i; j--) {
					GCareer.HighScore[GLevel][j].Score =
					    GCareer.HighScore[GLevel][j - 1].Score;
					GCareer.HighScore[GLevel][j].Skater =
					    GCareer.HighScore[GLevel][j - 1].Skater;

					{
						/* the counter is initialised between the two pointers (retail's t0/a3 order) */
						char *d = GCareer.HighScore[GLevel][j].Name;
						int k = 3;
						char *s = GCareer.HighScore[GLevel][j - 1].Name;

						for (; k != -1; k--)
							*d++ = *s++;
					}
				}

				GCareer.HighScore[GLevel][i].Score = pSkater->mPanel.mTotalScore;
				GCareer.HighScore[GLevel][i].Skater = GSkater->mSkaterIndex;
				FUN_8008b900(GCareer.HighScore[GLevel][i].Name,
				             GCareer.Options.LastHighScoreInitials);

				GHighScoreLineIndex = i;
				i = 8;
			}
		}
	}

	if (GGame == 7) {
		DAT_800a5b28[0].init();

		GHorseScale = 0;
		GHorseWaitTime = 45;
		GHorseContinue = 0;
	}

	CommonSwitchOnMenuCode__Fv();
	InitStatsSwisch();
}
