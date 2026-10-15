/* Competition_ResetMarquee__Fv (0x8007657c) */
#include "game/competition.h"
#include "game/career.h"
#include "game/psyq.h"

extern int Vblanks, SelectedSkater;
extern const int GLevel;
extern SLevel Levels[];

short HOLYCOW_PLACE, FIRST_PLACE, SECOND_PLACE, THIRD_PLACE, LAST_PLACE;

void Redbook_InitialRand(int);
int Redbook_Rnd(int);

void Competition_ResetMarquee() {
	CCompetition *c = &theCompetition;
	int n;

	if (c->mCurrentHeat == 0) {
		c->mJudgeTimer = 0;
		c->mLadderTimer = 0;
		c->mWinLoseCheer = 0;
		c->mFading = 0;
		Redbook_InitialRand(Vblanks);

		{
			int i;

			for (i = 0; i < 10; i++)
				c->mCompetitorList[i] = i;
		}

		n = SelectedSkater;
		c->our_skater_num = n;

		if (n == 10) {
			c->our_skater_num = Redbook_Rnd(10);
			c->mCompetitorList[c->our_skater_num] = n;
		}

		if (GLevel == 8) {
			HOLYCOW_PLACE = 975;
			FIRST_PLACE = 935;
			SECOND_PLACE = 910;
			THIRD_PLACE = 860;
			LAST_PLACE = 770;
		} else {
			HOLYCOW_PLACE = 975;
			FIRST_PLACE = 920;
			SECOND_PLACE = 870;
			THIRD_PLACE = 820;
			LAST_PLACE = 520;
		}

		{
			uchar i;

			for (i = 0; i < 5; i++) {
				c->mJudges[i].mProPlayer = Redbook_Rnd(10);
				c->mJudges[i].mConPlayer = Redbook_Rnd(10);
				c->mJudges[i].mProBias = Redbook_Rnd(30);
				c->mJudges[i].mConBias = Redbook_Rnd(30);
			}
		}

		{
			uchar i;

			for (i = 0; i < 10; i++) {
				uchar j;

				for (j = 0; j < 3; j++)
					c->mScore[i][j] = 0;

				c->mTotalScore[i] = 0;
				c->mOrdering[i] = i;
			}
		}

		c->mCurrentHeat = 0;

		{
			int i;

			for (i = 0; i < 10; i++)
				c->mPlacing[i] = 255;
		}

		{
			int i;

			for (i = 0; i < 10; i++) {
				int k, j, slot;

				slot = Redbook_Rnd(10 - i);
				j = 0;
				k = -1;

				while (slot >= j) {
					k++;

					if (c->mPlacing[k] == 255)
						j++;
				}

				c->mPlacing[k] = i;
			}
		}

		{
			int i;

			for (i = 0; i < 10; i++)
				if (c->mPlacing[i] == 9)
					c->mPlacing[i] = c->mPlacing[c->our_skater_num];
		}

		c->mPlacing[c->our_skater_num] = 255;
	}

	c->mMarqueeTimer = 0;
	c->mMarqueeText = 0;
	sprintf(c->mMarqueeTextBuf, "%s %s - HEAT %d", Levels[GLevel].pLevelName,
	        Levels[GLevel].pCityName, c->mCurrentHeat + 1);

	c->mMarqueeText = c->mMarqueeTextBuf;
}
