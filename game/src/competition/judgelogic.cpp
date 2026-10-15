/* Competition_JudgeLogic__Fv (0x80074350) */
/* C++ (COMPETITION.CPP). Demo twin (0.99); locals from its MAIN.SYM. On the first frame of the
   judging, each of the five judges scores the run: the skater's points against the level's three
   medal thresholds pick a band (interpolated between the band scores), minus a penalty scaled by
   a skater counter (+0x930),
   a random dip, and the judge's bias for or against our skater. The run's score is the average of
   the three middle judges. Gold/Silver/Bronze are read into locals (not in the SYM) and each band
   formula is two statements, as in the old draft; the shapes decide register use and cross-jumping. */
#include "game/competition.h"
#include "game/career.h"
#include "game/bruce.h"

extern SLevel Levels[];
extern CBruce *MechList;
extern int GLevel;

extern ushort HOLYCOW_PLACE;
extern ushort FIRST_PLACE;
extern ushort SECOND_PLACE;
extern ushort THIRD_PLACE;

int Redbook_Rnd(int);

void Competition_JudgeLogic() {
	CCompetition *c = &theCompetition;

	if (!c->mJudgeTimer) {
		CBruce *pSkater;
		uint temp;
		uint score;
		int average_score;

		pSkater = MechList;
		c->mJudgeFullyDisplayed = 0;
		score = pSkater->mPanel.mTotalScore;

		{
			int i;

			for (i = 0; i < 5; i++) {
				uint Gold = Levels[GLevel].GoldScore;

				if (Gold * 2 < score) {
					temp = ((score - Gold * 2) << 12) / (Gold * 3);
					temp = ((1000 - HOLYCOW_PLACE) * temp) >> 12;
					temp += HOLYCOW_PLACE;
				} else if (Gold < score) {
					temp = ((score - Gold) << 12) / Gold;
					temp = ((HOLYCOW_PLACE - FIRST_PLACE) * temp) >> 12;
					temp += FIRST_PLACE;
				} else {
					uint Silver = Levels[GLevel].SilverScore;

					if (Silver < score) {
						temp = ((score - Silver) << 12) / (Gold - Silver);
						temp = ((FIRST_PLACE - SECOND_PLACE) * temp) >> 12;
						temp += SECOND_PLACE;
					} else {
						uint Bronze = Levels[GLevel].BronzeScore;

						if (Bronze < score) {
							temp = ((score - Bronze) << 12) / (Silver - Bronze);
							temp = ((SECOND_PLACE - THIRD_PLACE) * temp) >> 12;
							temp += THIRD_PLACE;
						} else {
							temp = (score << 12) / Bronze;
							temp = ((THIRD_PLACE - 300) * temp) >> 12;
							temp += 300;
						}
					}
				}

				c->mJudgesScore[i] = temp;
				temp = pSkater->mPanel.mNumBails * (Redbook_Rnd(15) + 10);

				if (temp > 300)
					temp = 300;

				if (temp >= c->mJudgesScore[i])
					c->mJudgesScore[i] = 0;
				else
					c->mJudgesScore[i] -= temp;

				c->mJudgesScore[i] -= Redbook_Rnd(8);

				if (c->mJudges[i].mProPlayer == c->our_skater_num)
					c->mJudgesScore[i] += c->mJudges[i].mProBias;

				if (c->mJudges[i].mConPlayer == c->our_skater_num &&
				    c->mJudgesScore[i] > c->mJudges[i].mConBias)
					c->mJudgesScore[i] -= c->mJudges[i].mConBias;

				if (c->mJudgesScore[i] < 0)
					c->mJudgesScore[i] = 0;

				if (c->mJudgesScore[i] >= 1000)
					c->mJudgesScore[i] = 999;

				if (!score)
					c->mJudgesScore[i] = 0;
			}
		}

		c->mLowScore = 0;

		{
			int i;

			for (i = 0; i < 5; i++)
				if (c->mJudgesScore[i] < c->mJudgesScore[c->mLowScore])
					c->mLowScore = i;
		}

		if (!c->mLowScore)
			c->mHighScore = 1;
		else
			c->mHighScore = 0;

		{
			int i;

			for (i = 0; i < 5; i++)
				if (i != c->mLowScore && c->mJudgesScore[c->mHighScore] < c->mJudgesScore[i])
					c->mHighScore = i;
		}

		average_score = 0;

		{
			int i;

			for (i = 0; i < 5; i++)
				if (i != c->mLowScore && i != c->mHighScore)
					average_score += c->mJudgesScore[i];
		}

		average_score /= 3;
		c->mScore[c->our_skater_num][c->mCurrentHeat] = average_score;
	}

	c->mJudgeTimer++;
}
