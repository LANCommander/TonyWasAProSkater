/* Competition_LadderLogic__Fv (0x80074b94) */
#include "game/competition.h"

unsigned short FIRST_PLACE, SECOND_PLACE, THIRD_PLACE, LAST_PLACE;

int Redbook_Rnd(int n);

void Competition_LadderLogic() {
	unsigned char i, j;
	CCompetition *c = &theCompetition;
	unsigned short s;

	if (c->mLadderTimer == 0) {
		c->mLadderFullyDisplayed = 0;

		for (i = 0; i < 10; i++) {
			if (i == c->our_skater_num)
				continue;

			if (c->mPlacing[i] == 0)
				s = FIRST_PLACE;
			else if (c->mPlacing[i] == 1)
				s = SECOND_PLACE;
			else if (c->mPlacing[i] == 2)
				s = THIRD_PLACE;
			else
				s = LAST_PLACE + Redbook_Rnd(THIRD_PLACE - LAST_PLACE);

			s -= Redbook_Rnd(18);

			for (j = 0; j < 5; j++) {
				if (i == c->mJudges[j].mProPlayer)
					s += c->mJudges[j].mProBias;

				if (i == c->mJudges[j].mConPlayer)
					s -= c->mJudges[j].mConBias;
			}

			c->mScore[0][i * 3 + c->mCurrentHeat] = s;
		}

		c->mLastLadderEntry = 0;
	}

	c->mLadderTimer++;
}
