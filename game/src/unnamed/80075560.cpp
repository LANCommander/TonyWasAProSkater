/* Competition_WinLoseLogic__FR13EPausedStatusRi (0x80075560) flags=-fstructure-noalias */
#include "game/dbuffer.h"
#include "game/competition.h"
#include "game/powerup.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/front.h"
#include "game/pad.h"
#include "game/mem.h"

/* Competition win/lose logic (COMPETITION.CPP, demo Competition_WinLoseLogic); sibling of
   Competition_EndGameLogic and written the same way. */
enum EPausedStatus { STATUS_MEDALSAVE = 7, STATUS_ENDINGSCREEN = 14, STATUS_ENDGAME = 27 };

extern LocalText GameText;
extern CMenu *pYesNoMenu;
extern int GNumberOfPlayers;
extern int GLevel;
extern SCareer GCareer;
extern SLevel Levels[];
extern SControl Pad[2];
extern CBruce *GSkater;

void Crowd_Stop();
void Flash_Reset();
int Flash_FadeFinished();
void GameFMV_PlayMovie(int);
extern "C" int FUN_80073f9c();
extern "C" int FUN_80077124(CCompetition *);
void Flash_Fade(int, int);
void Flash_Update();
void GameFMV_StopFMV();
void Mess_DeleteAll();
void Pad_ActuatorOff(uchar, uchar);
void Pad_ClearAll();
void Pad_ClearTriggers(SControl *);
void Pause(int);

void Competition_WinLoseLogic(EPausedStatus &a0, int &a1) {
	int v, e;
	CBruce *gg;
	CCompetition *g = &theCompetition;

	if (g->mFading) {
		Flash_Update();

		if (Flash_FadeFinished()) {
			g->mFading = 0;
			pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));
			delete g->mMedal1;
			delete g->mMedal2;
			delete g->mMedal3;

			gg = GSkater;
			e = GLevel;
			g->mMedal1 = 0;
			g->mMedal2 = 0;
			g->mMedal3 = 0;
			GCareer.Options.MedalMessagesUsed[gg->mSkaterIndex] |= 1 << e;
			GameFMV_StopFMV();
			Pad_ClearAll();
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			Pad_ActuatorOff(0, 0);
			Pad_ActuatorOff(0, 1);
			Mess_DeleteAll();
			Pause(10);
			Mem_Delete(DoubleBuffer[0].Polys);
			GameFMV_PlayMovie(-11);
			Flash_Reset();

			{
				int size = DB_PACKET_MEMORY(Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00);

				DoubleBuffer[0].Polys = (unsigned char *)Mem_NewX(size * 2, 1, 1);
				DoubleBuffer[1].Polys = DoubleBuffer[0].Polys + size;
			}
			a0 = STATUS_ENDGAME;
		}
	} else {
		Crowd_Stop();

		if (a1) {
			if (FUN_80077124(g)) {
				v = 1;

				if (g->mMedal1) {
					int c = g->mMedal1->mTimer < 40;

					v = c == 0;
				}

				if (v) {
					if (FUN_80073f9c()) {
						g->mFading = 1;
						Flash_Fade(32, 0);
						return;
					}

					a0 = STATUS_MEDALSAVE;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));
				} else
					return;
			} else {
				a0 = STATUS_ENDINGSCREEN;
			}

			delete g->mMedal1;
			delete g->mMedal2;
			delete g->mMedal3;

			g->mMedal1 = 0;
			g->mMedal2 = 0;
			g->mMedal3 = 0;
			GCareer.Options.MedalMessagesUsed[GSkater->mSkaterIndex] |= 1 << GLevel;
		}
	}
}
