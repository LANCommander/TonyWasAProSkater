/* Competition_NewHeat__Fv (0x800764a4) */
#include "game/competition.h"

int Competition_NewHeat() {
	theCompetition.mLadderTimer = 0;
	theCompetition.mJudgeTimer = 0;
	theCompetition.mWinLoseCheer = 0;
	theCompetition.mJudgeFullyDisplayed = 0;
	theCompetition.mLadderFullyDisplayed = 0;
	return ++theCompetition.mCurrentHeat < 3;
}
