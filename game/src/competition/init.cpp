/* Competition_Init__Fv (0x800740b4) */
#include "game/competition.h"

extern int Vblanks, SelectedSkater, GLevel;

extern short HOLYCOW_PLACE, FIRST_PLACE, SECOND_PLACE, THIRD_PLACE, LAST_PLACE;

void Redbook_InitialRand(int);
int Redbook_Rnd(int);

void Competition_Init() {
	CCompetition *c = &theCompetition;
	unsigned char *p;
	unsigned char *q;
	int i, j, r, cnt, idx, t;
	unsigned char u, k, m;

	c->mJudgeTimer = 0;
	c->mLadderTimer = 0;
	c->mWinLoseCheer = 0;
	c->mFading = 0;
	Redbook_InitialRand(Vblanks);

	for (i = 0; i < 10; i++)
		c->mCompetitorList[i] = i;

	t = SelectedSkater;
	c->our_skater_num = t;

	if (t == 10) {
		c->our_skater_num = Redbook_Rnd(10);
		c->mCompetitorList[c->our_skater_num] = t;
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

	for (u = 0; u < 5; u++) {
		c->mJudges[u].mProPlayer = Redbook_Rnd(10);
		c->mJudges[u].mConPlayer = Redbook_Rnd(10);
		c->mJudges[u].mProBias = Redbook_Rnd(30);
		c->mJudges[u].mConBias = Redbook_Rnd(30);
	}

	for (k = 0; k < 10; k++) {
		for (m = 0; m < 3; m++)
			c->mScore[k][m] = 0;

		c->mTotalScore[k] = 0;
		c->mOrdering[k] = k;
	}

	c->mCurrentHeat = 0;

	for (i = 0; i < 10; i++)
		c->mPlacing[i] = 255;

	for (j = 0; j < 10; j++) {
		r = Redbook_Rnd(10 - j);
		cnt = 0;
		idx = -1;

		while (cnt <= r) {
			idx++;

			if (c->mPlacing[idx] == 255)
				cnt++;
		}

		c->mPlacing[idx] = j;
	}

	for (i = 0; i < 10; i++)
		if (c->mPlacing[i] == 9)
			c->mPlacing[i] = c->mPlacing[c->our_skater_num];

	c->mPlacing[c->our_skater_num] = 255;
}
