/* Competition_EndGameLogic__FR13EPausedStatusRi (0x80076880) flags=-fstructure-noalias */
#include "game/dbuffer.h"
#include "game/competition.h"
#include "game/powerup.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/front.h"
#include "game/pad.h"
#include "game/mem.h"

/* Competition end-of-game logic (COMPETITION.CPP, demo Competition_EndGameLogic): after the
   results flash, kills the medal objects, records the medal, and reloads/plays the ending; else
   waits for the prompt and offers the continue menu. */
enum EPausedStatus { STATUS_MEDALSAVE = 7 };

extern SCareer GCareer;
extern SLevel Levels[];
extern SControl Pad[2];
extern CBruce *GSkater;
extern int GLevel;
extern int GNumberOfPlayers;
extern int GRollCredits;
extern CMenu *pYesNoMenu;
extern LocalText GameText;

void Flash_Update();
int Flash_FadeFinished();
void GameFMV_StopFMV();
void Pad_ClearAll();
void Pad_ClearTriggers(SControl *);
void Pad_ActuatorOff(uchar, uchar);
void Mess_DeleteAll();
void Pause(int);
void GameFMV_PlayMovie(int);
void Flash_Reset();
void Flash_Fade(int, int);

void Competition_EndGameLogic(EPausedStatus &a, int &b) {
	CBruce *g;
	int e;

	if (theCompetition.mFading) {
		Flash_Update();

		if (Flash_FadeFinished()) {
			theCompetition.mFading = 0;

			delete theCompetition.mMedal1;
			delete theCompetition.mMedal2;
			delete theCompetition.mMedal3;

			e = GLevel;
			g = GSkater;
			theCompetition.mMedal1 = 0;
			theCompetition.mMedal2 = 0;
			theCompetition.mMedal3 = 0;
			GCareer.Options.MedalMessagesUsed[g->mSkaterIndex] |= 1 << e;
			GameFMV_StopFMV();
			Pad_ClearAll();
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			Pad_ActuatorOff(0, 0);
			Pad_ActuatorOff(0, 1);
			Mess_DeleteAll();
			Pause(10);
			Mem_Delete(DoubleBuffer[0].Polys);
			GameFMV_PlayMovie(-(GSkater->mSkaterIndex + 12));
			Flash_Reset();

			{
				int size = DB_PACKET_MEMORY(Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00);

				DoubleBuffer[0].Polys = (unsigned char *)Mem_NewX(size * 2, 1, 1);
				DoubleBuffer[1].Polys = DoubleBuffer[0].Polys + size;
			}
			a = STATUS_MEDALSAVE;
		}
	} else if (b) {
		int v = 1;

		if (theCompetition.mMedal1) {
			int c = theCompetition.mMedal1->mTimer < 40;

			v = c == 0;
		}

		if (v) {
			GRollCredits = 1;
			pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));

			if (Competition_WonGameAllGold()) {
				theCompetition.mFading = 1;
				Flash_Fade(32, 0);
				return;
			}

			a = STATUS_MEDALSAVE;

			delete theCompetition.mMedal1;
			delete theCompetition.mMedal2;
			delete theCompetition.mMedal3;

			theCompetition.mMedal1 = 0;
			theCompetition.mMedal2 = 0;
			theCompetition.mMedal3 = 0;
			GCareer.Options.MedalMessagesUsed[GSkater->mSkaterIndex] |= 1 << GLevel;
		}
	}
}
