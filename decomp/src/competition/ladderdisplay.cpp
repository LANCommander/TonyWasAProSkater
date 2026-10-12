/* Competition_LadderDisplay__Fv (0x80074d10) */
/* C++ (COMPETITION.CPP). Fuzzy demo twin; CCompetition and the locals from its MAIN.SYM (pComp isn't listed
   there, but retail keeps the global's address in s2 from entry). */
#include "game/competition.h"

struct SSkaterInfo {
	char *pName;
	char pad[0x154 - 4];
};

extern SSkaterInfo Stats_2c[];
extern char s_COMPETITION_STANDINGS[], s_Heat[], s_1_2_3_Total[];
extern char s_s_800d1fd4[];
extern char s_d_d[];
extern char DAT_800d1fe0[];
extern char *GameText_180[];

void Mess_DeleteAll();
void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
extern "C" void Mess_SetTextJustify__FUc(int);
extern "C" int FUN_8008b900(char *, char *, ...);
extern "C" int Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
int SFX_PlayX(unsigned int, short, int);
void Front_PrintCentredPrompt(const char *);

void Competition_LadderDisplay() {
	uchar b;
	uchar a;
	uchar i;
	char text_buf[45];
	CCompetition *pComp = &theCompetition;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetTextJustify__FUc(0);
	Mess_SetRGB(182, 132, 36, 0);
	FUN_8008b900(text_buf, s_COMPETITION_STANDINGS);
	Mess_DrawText__FiiPCciUiib(256, 40, text_buf, 0, 0, 256, 0);
	Mess_SetTextJustify__FUc(1);
	Mess_SetScale(120);
	FUN_8008b900(text_buf, s_Heat);
	Mess_DrawText__FiiPCciUiib(260, 50, text_buf, 0, 0, 256, 0);
	FUN_8008b900(text_buf, s_1_2_3_Total);
	Mess_DrawText__FiiPCciUiib(260, 60, text_buf, 0, 0, 256, 0);
	Mess_SetScale(160);

	for (i = 0; i < 10; i++) {
		FUN_8008b900(text_buf, s_s_800d1fd4,
		             Stats_2c[pComp->mCompetitorList[pComp->mOrdering[i]]].pName);

		if (pComp->mOrdering[i] == pComp->our_skater_num)
			Mess_SetRGB(182, 132, 36, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText__FiiPCciUiib(90, i * 14 + 70, text_buf, 0, 0, 256, 0);
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
					SFX_PlayX(52, 0x2000, 0);

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

				FUN_8008b900(text_buf, s_d_d, s / 10, s % 10);
				Mess_DrawText__FiiPCciUiib(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}

			if (a == pComp->mCurrentHeat) {
				if (b * 20 < pComp->mLadderTimer) {
					ushort s = pComp->mScore[pComp->mOrdering[b]][a];

					FUN_8008b900(text_buf, s_d_d, s / 10, s % 10);
				} else {
					if (pComp->mOrdering[b] == pComp->our_skater_num)
						Mess_SetRGB(182, 132, 36, 0);
					else
						Mess_SetRGB(122, 122, 122, 0);

					FUN_8008b900(text_buf, DAT_800d1fe0);
				}

				Mess_DrawText__FiiPCciUiib(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);

				if (pComp->mOrdering[b] == pComp->our_skater_num)
					Mess_SetRGB(182, 132, 36, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				if (b * 20 < pComp->mLadderTimer || pComp->mCurrentHeat) {
					ushort s = pComp->mTotalScore[pComp->mOrdering[b]];

					FUN_8008b900(text_buf, s_d_d, s / 10, s % 10);
				} else
					FUN_8008b900(text_buf, DAT_800d1fe0);

				Mess_DrawText__FiiPCciUiib(450, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}

			if (a > pComp->mCurrentHeat) {
				FUN_8008b900(text_buf, DAT_800d1fe0);
				Mess_DrawText__FiiPCciUiib(a * 60 + 250, b * 14 + 70, text_buf, 0, 0, 256, 0);
			}
		}
	}

	if (pComp->mLadderTimer > 200)
		pComp->mLadderFullyDisplayed = 1;
	else
		pComp->mLadderFullyDisplayed = 0;

	Mess_SetScale(256);

	if (pComp->mLadderFullyDisplayed)
		Front_PrintCentredPrompt(GameText_180[0]);
}
