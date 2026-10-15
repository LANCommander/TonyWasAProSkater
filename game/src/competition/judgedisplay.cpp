/* Competition_JudgeDisplay__Fv (0x800747b0) */
#include "game/competition.h"
#include "game/front.h"
#include "game/mess.h"
#include "game/sfx.h"
#include "game/psyq.h"

extern LocalText GameText;

void Mess_DeleteAll();
void Front_PrintCentredPrompt(const char *);

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
			SFX_PlayX(52, 8192, 0);

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
			SFX_PlayX(51, 8192, 0);

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
