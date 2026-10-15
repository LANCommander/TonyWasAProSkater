/* Competition_WinLoseDisplay__Fv (0x80075964) flags=-fstructure-noalias */
/* C++ (COMPETITION.CPP). Fuzzy demo twin (0.98); locals from its MAIN.SYM. End of a competition:
   simulates the AI skaters' remaining runs (base score by skill, a random dip, the judges'
   biases; best two of three runs count), sorts the standings, then shows our result (a medal
   for the top three, else the podium and our place), the medal unlock text and the prompt. */
#include "game/competition.h"
#include "game/powerup.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/mess.h"
#include "game/sfx.h"
#include "game/psyq.h"

extern char *MedalMessage[9];
extern SStats Stats[];
extern SCareer GCareer;
extern CBruce *GSkater;
extern int GLevel;
extern char *GameText[];

extern ushort FIRST_PLACE;
extern ushort SECOND_PLACE;
extern ushort THIRD_PLACE;
extern ushort LAST_PLACE;

void Mess_DeleteAll(void);
void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
int Redbook_Rnd(int);
CPowerUp *Medal_Create(EMedalType, int, int, int, int, int);
void Front_PrintCentredPrompt(const char *);

void Competition_WinLoseDisplay() {
	int our_placing;
	char buf[40];
	CCompetition *c = &theCompetition;

	Mess_DeleteAll();

	while (c->mCurrentHeat < 3) {
		uchar i;
		int b;

		/* no for-increment: g++ puts a deleted note after a for's continue label, which stops
		   reorg predicting the skip as taken (retail fills its delay slot from the target) */
		i = (uchar)-1;

		while (++i < 10) {
			ushort base;
			uchar j;

			if (i == c->our_skater_num)
				continue;

			if (c->mPlacing[i] == 0)
				base = FIRST_PLACE;
			else if (c->mPlacing[i] == 1)
				base = SECOND_PLACE;
			else if (c->mPlacing[i] == 2)
				base = THIRD_PLACE;
			else
				base = Redbook_Rnd(THIRD_PLACE - LAST_PLACE) + LAST_PLACE;

			base -= Redbook_Rnd(18);

			for (j = 0; j < 5; j++) {
				CCompetition *p = (CCompetition *)((char *)c + j * sizeof(CJudge));

				if (i == p->mJudges[0].mProPlayer)
					base += p->mJudges[0].mProBias;

				if (i == p->mJudges[0].mConPlayer)
					base -= p->mJudges[0].mConBias;
			}

			c->mScore[i][c->mCurrentHeat] = base;
		}

		c->mCurrentHeat++;

		for (b = 0; b < 10; b++) {
			uint score0 = c->mScore[b][0];
			uint score1 = c->mScore[b][1];
			uint score2 = c->mScore[b][2];

			if (score0 < score1 && score0 < score2)
				c->mTotalScore[b] = score1 + score2;
			else if (score1 < score0 && score1 < score2)
				c->mTotalScore[b] = score0 + score2;
			else
				c->mTotalScore[b] = score0 + score1;
		}
	}

	{
		int i;

		for (i = 0; i < 10; i++)
			c->mPlacing[i] = i;
	}

	{
		int i;

		for (i = 1; i < 10; i++) {
			int j = i;

			while (c->mTotalScore[c->mPlacing[j]] > c->mTotalScore[c->mPlacing[j - 1]]) {
				uchar t = c->mPlacing[j];
				uchar u = c->mPlacing[j - 1];

				c->mPlacing[j - 1] = t;
				c->mPlacing[j] = u;

				if (--j <= 0)
					break;
			}
		}
	}

	Mess_SetScale(256);
	Mess_SetRGB(122, 122, 122, 0);

	{
		int i;

		for (i = 0; i < 10; i++)
			if (c->mPlacing[i] == c->our_skater_num)
				our_placing = i;
	}

	switch (our_placing) {
		case 0:
			Mess_DrawText(256, 40, "You win the Gold!", 0, 0, 256, 0);

			if (!c->mMedal1) {
				Career_GiveMedal(GLevel, 5);

				c->mMedal1 = Medal_Create((EMedalType)5, 0, -5, 140, 2048, 10);
			}

			if (!c->mWinLoseCheer) {
				SFX_PlayX(45, 0x2000, 0);
				SFX_PlayX(48, 0x2000, 0);

				c->mWinLoseCheer = 1;
			}

			break;
		case 1:
			Mess_DrawText(256, 40, "You win the Silver!", 0, 0, 256, 0);

			if (!c->mMedal1) {
				Career_GiveMedal(GLevel, 6);

				c->mMedal1 = Medal_Create((EMedalType)6, 0, -5, 140, 2048, 10);
			}

			if (!c->mWinLoseCheer) {
				SFX_PlayX(48, 0x2000, 0);

				c->mWinLoseCheer = 1;
			}

			break;
		case 2:
			Mess_DrawText(256, 40, "You win the Bronze!", 0, 0, 256, 0);

			if (!c->mMedal1) {
				Career_GiveMedal(GLevel, 7);

				c->mMedal1 = Medal_Create((EMedalType)7, 0, -5, 140, 2048, 10);
			}

			if (!c->mWinLoseCheer) {
				SFX_PlayX(45, 0x2000, 0);

				c->mWinLoseCheer = 1;
			}

			break;
		default: {
			int first;
			int second;
			int third;

			if (!c->mWinLoseCheer) {
				SFX_PlayX(47, 0x2000, 0);

				c->mWinLoseCheer = 1;
			}

			Mess_DrawText(256, 40, "You lose!", 0, 0, 256, 0);

			{
				int i;

				for (i = 0; i < 10; i++) {
					if (c->mPlacing[0] == i)
						first = i;

					if (c->mOrdering[1] == i)
						second = i;

					if (c->mOrdering[2] == i)
						third = i;
				}
			}

			Mess_SetRGB(182, 132, 36, 0);
			Mess_SetScale(160);
			sprintf(buf, "%s: 1st place", Stats[c->mCompetitorList[first]].Name);
			Mess_DrawText(256, 60, buf, 0, 0, 256, 0);
			sprintf(buf, "%s: 2nd place", Stats[c->mCompetitorList[second]].Name);
			Mess_DrawText(256, 70, buf, 0, 0, 256, 0);
			sprintf(buf, "%s: 3rd place", Stats[c->mCompetitorList[third]].Name);
			Mess_DrawText(256, 80, buf, 0, 0, 256, 0);
			Mess_SetRGB(122, 122, 122, 0);
			sprintf(buf, "%dth", our_placing + 1);
			Mess_SetScale(384);
			Mess_DrawText(256, 120, Stats[c->mCompetitorList[c->our_skater_num]].Name, 0, 0, 256,
			              0);
			Mess_DrawText(256, 150, buf, 0, 0, 256, 0);
			break;
		}
	}

	if (c->mMedal1) {
		c->mMedal1->AI();

		c->medal_mask = 0;

		if (GCareer.CharacterProgress[GSkater->mSkaterIndex].Tapes[GLevel] & 0xe0)
			c->medal_mask = 1 << GLevel;

		Mess_SetScale(256);
		Mess_SetRGB(122, 122, 122, 0);

		if (GCareer.Options.MedalMessagesUsed[GSkater->mSkaterIndex] == 328) {
			c->mMedal1->mMaxWobbleAmplitude = c->mMedal1->mWobbleAmplitude = 2;
			c->mMedal1->mPos.vy = c->mMedal1->mOrgPos.vy - 0x5000;

			if (MedalMessage[6])
				Mess_DrawText(256, 155, MedalMessage[6], 0, 0, 256, 0);

			if (MedalMessage[7])
				Mess_DrawText(256, 175, MedalMessage[7], 0, 0, 256, 0);

			if (MedalMessage[8])
				Mess_DrawText(256, 195, MedalMessage[8], 0, 0, 256, 0);
		} else {
			switch ((GCareer.Options.MedalMessagesUsed[GSkater->mSkaterIndex] & c->medal_mask) ^
			        c->medal_mask) {
				case 8:
					if (MedalMessage[0])
						Mess_DrawText(256, 155, MedalMessage[0], 0, 0, 256, 0);

					if (MedalMessage[1])
						Mess_DrawText(256, 175, MedalMessage[1], 0, 0, 256, 0);

					if (MedalMessage[2])
						Mess_DrawText(256, 195, MedalMessage[2], 0, 0, 256, 0);

					break;
				case 64:
					if (MedalMessage[3])
						Mess_DrawText(256, 155, MedalMessage[3], 0, 0, 256, 0);

					if (MedalMessage[4])
						Mess_DrawText(256, 175, MedalMessage[4], 0, 0, 256, 0);

					if (MedalMessage[5])
						Mess_DrawText(256, 195, MedalMessage[5], 0, 0, 256, 0);

					break;
				case 256:
					c->mMedal1->mMaxWobbleAmplitude = c->mMedal1->mWobbleAmplitude = 2;
					c->mMedal1->mPos.vy = c->mMedal1->mOrgPos.vy - 0x5000;

					if (MedalMessage[6])
						Mess_DrawText(256, 155, MedalMessage[6], 0, 0, 256, 0);

					if (MedalMessage[7])
						Mess_DrawText(256, 175, MedalMessage[7], 0, 0, 256, 0);

					if (MedalMessage[8])
						Mess_DrawText(256, 195, MedalMessage[8], 0, 0, 256, 0);

					break;
			}
		}

		{
			int show = 1;

			if (c->mMedal1 && c->mMedal1->mTimer < 40)
				show = 0;

			if (show && !c->mFading)
				Front_PrintCentredPrompt(GameText[96]);
		}
	} else {
		if (!c->mFading)
			Front_PrintCentredPrompt(GameText[96]);
	}
}
