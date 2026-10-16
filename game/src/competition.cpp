/* COMPETITION.CPP: the competition levels' heats, judges, ladder and medal screens, around the
   single global theCompetition (CCompetition / CJudge, declarations in game/competition.h). */
#include "game/competition.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/front.h"
#include "game/mess.h"
#include "game/sfx.h"
#include "game/psyq.h"
#include "game/dbuffer.h"
#include "game/powerup.h"
#include "game/pad.h"
#include "game/mem.h"
#include "game/crowd.h"
#include "game/flash.h"
#include "game/gamefmv.h"
#include "game/globals.h"
#include "game/medals.h"
#include "game/utils.h"

/* the judges' score bands (Competition_Init sets them per level) */
extern ushort HOLYCOW_PLACE, FIRST_PLACE, SECOND_PLACE, THIRD_PLACE, LAST_PLACE;
extern "C" int FUN_80073f9c();
extern "C" int FUN_80077124(CCompetition *);
extern char *MedalMessage[9];
/* The competition state, built by the module's static constructor (_GLOBAL_.I, 0x800770dc) and
   torn down by FUN_80077060. CCompetition's destructor is the implicit one, which never re-stores
   its own vptr; only the inlined ~CClass store (0x800a3464) remains. The out-of-line copy is
   FUN_80077130. */
CCompetition theCompetition;
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};
extern char DAT_800a3464[];
void __dl__6CClassPv(void *p);

/* Competition_WonGameAllGold__Fv (0x8007401c) */
/* C++. Whether the skater has a gold medal (bit 5 of the level's career byte) in all three
   competition levels. */
int Competition_WonGameAllGold() {
	int c = 0;
	int i = 0;
	CBruce *g = GSkater[0];

	/* (the game's own competitions: the ending is theirs) */
	for (; i < STOCK_LEVELS; i++) {
		if (Levels[i].isCompetition) {
			if (*Career_Tapes(g->mSkaterIndex, i) & 32)
				c++;
		}
	}

	return c == 3;
}

/* Competition_IsOver__Fv (0x8007409c) */
int Competition_IsOver() {
	return theCompetition.mCurrentHeat >= 3;
}

/* Competition_Init__Fv (0x800740b4) */
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
	Utils_InitialRand(Vblanks);

	for (i = 0; i < 10; i++)
		c->mCompetitorList[i] = i;

	t = SelectedSkater;
	c->our_skater_num = t;

	/* (Officer Dick, or a mod's skater: not one of the ten competitors) */
	if (t >= 10) {
		c->our_skater_num = Rnd(10);
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
		c->mJudges[u].mProPlayer = Rnd(10);
		c->mJudges[u].mConPlayer = Rnd(10);
		c->mJudges[u].mProBias = Rnd(30);
		c->mJudges[u].mConBias = Rnd(30);
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
		r = Rnd(10 - j);
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

/* Competition_JudgeLogic__Fv (0x80074350) */
/* Demo twin (0.99); locals from its MAIN.SYM. On the first frame of the
   judging, each of the five judges scores the run: the skater's points against the level's three
   medal thresholds pick a band (interpolated between the band scores), minus a penalty scaled by
   a skater counter (+0x930),
   a random dip, and the judge's bias for or against our skater. The run's score is the average of
   the three middle judges. Gold/Silver/Bronze are read into locals (not in the SYM) and each band
   formula is two statements, as in the old draft; the shapes decide register use and cross-jumping. */
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
				temp = pSkater->mPanel.mNumBails * (Rnd(15) + 10);

				if (temp > 300)
					temp = 300;

				if (temp >= c->mJudgesScore[i])
					c->mJudgesScore[i] = 0;
				else
					c->mJudgesScore[i] -= temp;

				c->mJudgesScore[i] -= Rnd(8);

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

/* Competition_JudgeDisplay__Fv (0x800747b0) */
void Competition_JudgeDisplay() {
	char buf[16];
	CCompetition *p = &theCompetition;
	int i, t, c, sum, k;
	unsigned s;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetTextJustify(0);
	Mess_SetRGB(182, 132, 36, 0);
	sprintf(buf, "HEAT %d OVER!", p->mCurrentHeat + 1);
	Mess_DrawText(256, 40, buf, 0, 0, 256, 0);
	Mess_DrawText(256, 70, "JUDGES SCORES", 0, 0, 256, 0);
	Mess_SetRGB(122, 122, 122, 0);
	Mess_SetTextJustify(1);

	t = 0;

	if (p->mJudgeTimer >= 76)
		t = (p->mJudgeTimer - 75) << 2;

	if (t > 100)
		t = 100;

	c = (122 - t) & 255;

	for (i = 0; i < 5; i++) {
		s = i * 15;

		if (p->mJudgeTimer == s + 1)
			SFX_PlayX(SFX_JUDGE_SCORE, 8192, 0);

		if (s < p->mJudgeTimer) {
			sprintf(buf, "JUDGE %d     %d.%d", i + 1, p->mJudgesScore[i] / 10, p->mJudgesScore[i] % 10);
		} else {
			sprintf(buf, "JUDGE %d", i + 1);
		}

		if (i == p->mLowScore || i == p->mHighScore)
			Mess_SetRGB(c, c, c, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText(150, i * 20 + 90, buf, 0, 0, 256, 0);
	}

	if (t >= 100) {
		if (p->mJudgeFullyDisplayed == 0)
			SFX_PlayX(SFX_JUDGE_TOTAL, 8192, 0);

		Mess_SetTextJustify(0);
		Mess_SetRGB(182, 132, 36, 0);

		sum = 0;

		for (k = 0; k < 5; k++)
			if (k != p->mLowScore && k != p->mHighScore)
				sum += p->mJudgesScore[k];

		sum = sum / 3;
		Mess_SetTextJustify(0);
		sprintf(buf, "HEAT %d SCORE - ", p->mCurrentHeat + 1);
		Mess_DrawText(200, 195, buf, 0, 0, 256, 0);
		sprintf(buf, "%d.%d", sum / 10, sum % 10);
		Mess_SetScale(384);
		Mess_DrawText(335, 195, buf, 0, 0, 256, 0);

		p->mJudgeFullyDisplayed = 1;
	} else {
		p->mJudgeFullyDisplayed = 0;
	}

	Mess_SetScale(256);
	Front_PrintCentredPrompt(GameText.pContinue);
}

/* Competition_LadderLogic__Fv (0x80074b94) */
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
				s = LAST_PLACE + Rnd(THIRD_PLACE - LAST_PLACE);

			s -= Rnd(18);

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

/* Competition_LadderDisplay__Fv (0x80074d10) */
/* Fuzzy demo twin; CCompetition and the locals from its MAIN.SYM (pComp isn't listed
   there, but retail keeps the global's address in s2 from entry). */
void Competition_LadderDisplay() {
	uchar b;
	uchar a;
	uchar i;
	char text_buf[45];
	CCompetition *pComp = &theCompetition;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetTextJustify(0);
	Mess_SetRGB(182, 132, 36, 0);
	sprintf(text_buf, "COMPETITION STANDINGS");
	Mess_DrawText(256, 40, text_buf, 0, 0, 256, 0);
	Mess_SetTextJustify(1);
	Mess_SetScale(120);
	sprintf(text_buf, "        Heat");
	Mess_DrawText(260, 50, text_buf, 0, 0, 256, 0);
	sprintf(text_buf, "1         2         3           Total");
	Mess_DrawText(260, 60, text_buf, 0, 0, 256, 0);
	Mess_SetScale(160);

	for (i = 0; i < 10; i++) {
		sprintf(text_buf, "%s", Stats[pComp->mCompetitorList[pComp->mOrdering[i]]].Name);

		if (pComp->mOrdering[i] == pComp->our_skater_num)
			Mess_SetRGB(182, 132, 36, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText(90, i * 14 + 70, text_buf, 0, 0, 256, 0);
	}

	for (a = 0; a < 3; a++) {
		for (b = 0; b < 10; b++) {
			uint low_score;
			uint score2;
			uint score1;
			uint score0;
			int dimness;

			if (b * 20 < pComp->mLadderTimer) {
				if (pComp->mLadderTimer == b * 20 + 1)
					SFX_PlayX(SFX_JUDGE_SCORE, 0x2000, 0);

				if (pComp->mCurrentHeat == 0) {
					pComp->mTotalScore[pComp->mOrdering[b]] = pComp->mScore[pComp->mOrdering[b]][0];
				} else {
					uint score2;
					uint score1;
					uint score0;

					score0 = pComp->mScore[pComp->mOrdering[b]][0];
					score1 = pComp->mScore[pComp->mOrdering[b]][1];
					score2 = pComp->mScore[pComp->mOrdering[b]][2];

					if (score0 < score1 && score0 < score2)
						pComp->mTotalScore[pComp->mOrdering[b]] = score1 + score2;
					else if (score1 < score0 && score1 < score2)
						pComp->mTotalScore[pComp->mOrdering[b]] = score0 + score2;
					else
						pComp->mTotalScore[pComp->mOrdering[b]] = score0 + score1;
				}
			}

			if (b && b * 20 < pComp->mLadderTimer) {
				int j = b;

				do {
					if (!(pComp->mTotalScore[pComp->mOrdering[j]] >
					      pComp->mTotalScore[pComp->mOrdering[j - 1]]))
						break;

					{
						int temp_b = pComp->mOrdering[j - 1];

						pComp->mOrdering[j - 1] = pComp->mOrdering[j];
						pComp->mOrdering[j] = temp_b;
					}
				} while (--j > 0);
			}

			dimness = 0;

			if (pComp->mCurrentHeat == 2) {
				if (pComp->mLadderTimer < (b + 1) * 20) {
					if (pComp->mLadderTimer >= b * 20)
						dimness = (pComp->mLadderTimer % 20) * 4;
				} else if (pComp->mLadderTimer >= b * 20)
					dimness = 100;
			}

			low_score = 0;
			score0 = pComp->mScore[pComp->mOrdering[b]][0];
			score1 = pComp->mScore[pComp->mOrdering[b]][1];
			score2 = pComp->mScore[pComp->mOrdering[b]][2];

			if (score1 < score0 || score2 < score0) {
				low_score = 2;

				if (score0 >= score1 && score2 >= score1)
					low_score = 1;
			}

			if (a == low_score)
				Mess_SetRGB(122 - dimness, 122 - dimness, 122 - dimness, 0);
			else if (pComp->mOrdering[b] == pComp->our_skater_num)
				Mess_SetRGB(182, 132, 36, 0);
			else
				Mess_SetRGB(122, 122, 122, 0);

			if (a < pComp->mCurrentHeat) {
				ushort s = pComp->mScore[pComp->mOrdering[b]][a];

				sprintf(text_buf, "%d.%d", s / 10, s % 10);
				Mess_DrawText(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}

			if (a == pComp->mCurrentHeat) {
				if (b * 20 < pComp->mLadderTimer) {
					ushort s = pComp->mScore[pComp->mOrdering[b]][a];

					sprintf(text_buf, "%d.%d", s / 10, s % 10);
				} else {
					if (pComp->mOrdering[b] == pComp->our_skater_num)
						Mess_SetRGB(182, 132, 36, 0);
					else
						Mess_SetRGB(122, 122, 122, 0);

					sprintf(text_buf, "-");
				}

				Mess_DrawText(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);

				if (pComp->mOrdering[b] == pComp->our_skater_num)
					Mess_SetRGB(182, 132, 36, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				if (b * 20 < pComp->mLadderTimer || pComp->mCurrentHeat) {
					ushort s = pComp->mTotalScore[pComp->mOrdering[b]];

					sprintf(text_buf, "%d.%d", s / 10, s % 10);
				} else
					sprintf(text_buf, "-");

				Mess_DrawText(450, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}

			if (a > pComp->mCurrentHeat) {
				sprintf(text_buf, "-");
				Mess_DrawText(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}
		}
	}

	if (pComp->mLadderTimer > 200)
		pComp->mLadderFullyDisplayed = 1;
	else
		pComp->mLadderFullyDisplayed = 0;

	Mess_SetScale(256);

	if (pComp->mLadderFullyDisplayed)
		Front_PrintCentredPrompt(GameText.pContinue);
}

/* Competition_WinLoseLogic__FR13EPausedStatusRi (0x80075560) */
/* Competition win/lose logic (COMPETITION.CPP, demo Competition_WinLoseLogic); sibling of
   Competition_EndGameLogic and written the same way. */
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

			gg = GSkater[0];
			e = GLevel;
			g->mMedal1 = 0;
			g->mMedal2 = 0;
			g->mMedal3 = 0;
			*Career_MedalMessages(gg->mSkaterIndex) |= 1 << e;
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
			*Career_MedalMessages(GSkater[0]->mSkaterIndex) |= 1 << GLevel;
		}
	}
}

/* Competition_WinLoseDisplay__Fv (0x80075964) */
/* Fuzzy demo twin (0.98); locals from its MAIN.SYM. End of a competition:
   simulates the AI skaters' remaining runs (base score by skill, a random dip, the judges'
   biases; best two of three runs count), sorts the standings, then shows our result (a medal
   for the top three, else the podium and our place), the medal unlock text and the prompt. */
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
				base = Rnd(THIRD_PLACE - LAST_PLACE) + LAST_PLACE;

			base -= Rnd(18);

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
				SFX_PlayX(SFX_CROWD_APPLAUSE, 0x2000, 0);
				SFX_PlayX(SFX_CROWD_CHEER, 0x2000, 0);

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
				SFX_PlayX(SFX_CROWD_CHEER, 0x2000, 0);

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
				SFX_PlayX(SFX_CROWD_APPLAUSE, 0x2000, 0);

				c->mWinLoseCheer = 1;
			}

			break;
		default: {
			int first;
			int second;
			int third;

			if (!c->mWinLoseCheer) {
				SFX_PlayX(SFX_CROWD_AWW, 0x2000, 0);

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

		if (*Career_Tapes(GSkater[0]->mSkaterIndex, GLevel) & 0xe0)
			c->medal_mask = 1 << GLevel;

		Mess_SetScale(256);
		Mess_SetRGB(122, 122, 122, 0);

		if (*Career_MedalMessages(GSkater[0]->mSkaterIndex) == 328) {
			c->mMedal1->mMaxWobbleAmplitude = c->mMedal1->mWobbleAmplitude = 2;
			c->mMedal1->mPos.vy = c->mMedal1->mOrgPos.vy - 0x5000;

			if (MedalMessage[6])
				Mess_DrawText(256, 155, MedalMessage[6], 0, 0, 256, 0);

			if (MedalMessage[7])
				Mess_DrawText(256, 175, MedalMessage[7], 0, 0, 256, 0);

			if (MedalMessage[8])
				Mess_DrawText(256, 195, MedalMessage[8], 0, 0, 256, 0);
		} else {
			switch ((*Career_MedalMessages(GSkater[0]->mSkaterIndex) & c->medal_mask) ^
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
				Front_PrintCentredPrompt(GameText.pContinue);
		}
	} else {
		if (!c->mFading)
			Front_PrintCentredPrompt(GameText.pContinue);
	}
}

/* Competition_JudgeFullyDisplayed__Fv (0x80076464) */
int Competition_JudgeFullyDisplayed() {
	return theCompetition.mJudgeFullyDisplayed;
}

/* Competition_LadderFullyDisplayed__Fv (0x80076474) */
int Competition_LadderFullyDisplayed() {
	return theCompetition.mLadderFullyDisplayed;
}

/* Competition_FullyDisplayJudge__Fv (0x80076484) */
void Competition_FullyDisplayJudge() {
	theCompetition.mJudgeTimer = 1000;
}

/* Competition_FullyDisplayLadder__Fv (0x80076494) */
void Competition_FullyDisplayLadder() {
	theCompetition.mLadderTimer = 1000;
}

/* Competition_NewHeat__Fv (0x800764a4) */
int Competition_NewHeat() {
	theCompetition.mLadderTimer = 0;
	theCompetition.mJudgeTimer = 0;
	theCompetition.mWinLoseCheer = 0;
	theCompetition.mJudgeFullyDisplayed = 0;
	theCompetition.mLadderFullyDisplayed = 0;
	return ++theCompetition.mCurrentHeat < 3;
}

/* Competition_MarqueeDisplay__Fv (0x800764d8) */
void Competition_MarqueeDisplay() {
	if (theCompetition.mMarqueeText) {
		Mess_SetRGB(122, 122, 122, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(1);

		if (theCompetition.mMarqueeTimer > 768)
			theCompetition.mMarqueeTimer = 0;

		Mess_DrawText(512 - theCompetition.mMarqueeTimer, 200, theCompetition.mMarqueeText, 0, 0, 256, 0);

		theCompetition.mMarqueeTimer += 5;
	}
}

/* Competition_ResetMarquee__Fv (0x8007657c) */
void Competition_ResetMarquee() {
	CCompetition *c = &theCompetition;
	int n;

	if (c->mCurrentHeat == 0) {
		c->mJudgeTimer = 0;
		c->mLadderTimer = 0;
		c->mWinLoseCheer = 0;
		c->mFading = 0;
		Utils_InitialRand(Vblanks);

		{
			int i;

			for (i = 0; i < 10; i++)
				c->mCompetitorList[i] = i;
		}

		n = SelectedSkater;
		c->our_skater_num = n;

		if (n >= 10) {
			c->our_skater_num = Rnd(10);
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
				c->mJudges[i].mProPlayer = Rnd(10);
				c->mJudges[i].mConPlayer = Rnd(10);
				c->mJudges[i].mProBias = Rnd(30);
				c->mJudges[i].mConBias = Rnd(30);
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

				slot = Rnd(10 - i);
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

/* Competition_EndGameLogic__FR13EPausedStatusRi (0x80076880) */
/* Competition end-of-game logic (COMPETITION.CPP, demo Competition_EndGameLogic): after the
   results flash, kills the medal objects, records the medal, and reloads/plays the ending; else
   waits for the prompt and offers the continue menu. */
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
			g = GSkater[0];
			theCompetition.mMedal1 = 0;
			theCompetition.mMedal2 = 0;
			theCompetition.mMedal3 = 0;
			*Career_MedalMessages(g->mSkaterIndex) |= 1 << e;
			GameFMV_StopFMV();
			Pad_ClearAll();
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			Pad_ActuatorOff(0, 0);
			Pad_ActuatorOff(0, 1);
			Mess_DeleteAll();
			Pause(10);
			Mem_Delete(DoubleBuffer[0].Polys);
			GameFMV_PlayMovie(-(GSkater[0]->mSkaterIndex + 12));
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
			*Career_MedalMessages(GSkater[0]->mSkaterIndex) |= 1 << GLevel;
		}
	}
}

/* Competition_EndGameDisplay__Fv (0x80076c4c) */
/* Demo twin (0.98 by opcode); locals from its MAIN.SYM. The end-of-
   competition screen: either the "no medal" text with three bronze placeholders, or the results
   text and the medals won on the ten tracked events, spun in place, then the continue prompt once
   the first medal has settled. */
void Competition_EndGameDisplay() {
	CCompetition *pComp = &theCompetition;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetRGB(122, 122, 122, 0);
	Mess_SetTextJustify(0);

	if (Competition_WonGameAllGold()) {
		Mess_DrawText(256, 40, "You scored all golds!", 0, 0, 256, 0);
		Mess_DrawText(256, 60, "Your are the top pro skater.", 0, 0, 256, 0);
		Mess_DrawText(256, 80, "Congratulations!", 0, 0, 256, 0);
		Mess_DrawText(256, 180, "Watch this, then get off your", 0, 0, 256, 0);
		Mess_DrawText(256, 200, "couch and go skate!", 0, 0, 256, 0);

		if (!pComp->mMedal1)
			pComp->mMedal1 = Medal_Create((EMedalType)5, -30, 5, 140, 1536, 3);

		if (!pComp->mMedal2)
			pComp->mMedal2 = Medal_Create((EMedalType)5, 0, 5, 140, 1536, 3);

		if (!pComp->mMedal3)
			pComp->mMedal3 = Medal_Create((EMedalType)5, 30, 5, 140, 1536, 3);
	} else {
		Mess_DrawText(256, 40, "Your third medal.. Good going!", 0, 0, 256, 0);
		Mess_DrawText(256, 60, "You just might become", 0, 0, 256, 0);
		Mess_DrawText(256, 80, "an average skater.", 0, 0, 256, 0);
		Mess_DrawText(256, 180, "pro skaters get all golds...", 0, 0, 256, 0);

		{
			EMedalType m[3];
			int n = 0;
			int i;

			for (i = 0; i < STOCK_LEVELS; i++) {
				short f = *Career_Tapes(GSkater[0]->mSkaterIndex, i);

				if (f & 0x20)
					m[n++] = (EMedalType)5;
				else if (f & 0x40)
					m[n++] = (EMedalType)6;
				else if (f & 0x80)
					m[n++] = (EMedalType)7;
			}

			if (!pComp->mMedal1)
				pComp->mMedal1 = Medal_Create(m[0], -30, 5, 140, 1280, 5);

			if (!pComp->mMedal2)
				pComp->mMedal2 = Medal_Create(m[1], 0, 5, 140, 1280, 5);

			if (!pComp->mMedal3)
				pComp->mMedal3 = Medal_Create(m[2], 30, 5, 140, 1280, 5);
		}
	}

	pComp->mMedal1->AI();
	pComp->mMedal2->AI();
	pComp->mMedal3->AI();

	{
		int Done = 1;

		if (pComp->mMedal1) {
			int c = pComp->mMedal1->mTimer < 40;

			Done = c == 0;
		}

		if (Done && !pComp->mFading)
			Front_PrintCentredPrompt(GameText.pContinue);
	}
}

/* FUN_80077060 (0x80077060) */
/* The static teardown of theCompetition (_GLOBAL_.D): retail's cc1 emitted it for the global
   object; a native C++ build registers the destructor with atexit instead and has no function to
   put at this address. The body is FUN_80077130 applied to theCompetition, without the delete. */
extern "C" void FUN_80077060() {
	CCompetition *t = &theCompetition;
	CJudge *first = t->mJudges, *e;
	struct VtEntry *vt;

	if (first) {
		for (e = first + 5; first != e;) {
			e--;
			vt = *(struct VtEntry **)e; /* (CClass's vptr is at +0) */
			vt[1].fn((char *)e + vt[1].delta, 2);
		}
	}

	*(char **)t = DAT_800a3464;
}

/* FUN_80077124 (0x80077124) */
extern "C" int FUN_80077124(CCompetition *p) {
	return p->mMedal1 != 0;
}

/* FUN_80077130 (0x80077130) */
/* C++ for the port. The matching source is unnamed/80077130.cpp: there the destructor is the
   implicit one of a class cc1 only synthesizes as a stand-in, which a native build can't tie back
   to this address. This is the destructor of the competition object (theCompetition, see
   FUN_80077060): its five polymorphic judges are destroyed last to first through their vtables,
   then the inlined ~CClass restores CClass's vtable. */
extern "C" void FUN_80077130(CCompetition *t, int f) {
	CJudge *first = t->mJudges, *e;
	struct VtEntry *vt;

	if (first) {
		for (e = first + 5; first != e;) {
			e--;
			vt = *(struct VtEntry **)e; /* (CClass's vptr is at +0) */
			vt[1].fn((char *)e + vt[1].delta, 2);
		}
	}

	*(char **)t = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv((char *)t);
}

/* FUN_800771c4 (0x800771c4) */
void FUN_800771c4(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_800771f8 (0x800771f8) */
void FUN_800771f8(char *p, int f) {
	*(char **)(p + 0) = DAT_800a3464;

	if (f & 1)
		__dl__6CClassPv(p);
}
