/* Competition_IsOver__Fv (0x8007409c) */
#include "game/competition.h"

int Competition_IsOver() {
	return theCompetition.mCurrentHeat >= 3;
}
