/* DisplayScore__FP6CBruceUsUs (0x8006ca78) */
/* C++ (PANEL.CPP), from the demo twin + its MAIN.SYM locals (DoDisplay; block r/g/b; block score[15]
   with inner Value). Draws the balance meter and score panel, counts ScoreToAdd into Score, awards
   the two score tapes in career mode, and prints the score (flashing above 999999).
   Levers: DrawGouraudPoly returns POLY_G4* and Mess_DrawPanelText returns int (register choice of
   the stacked args); GHorseLetters is a small sized extern ([2]) so its address is one `la` and the
   Horse_DisplayWords jal delay slot stays empty; Sprite::draw takes ints (no &255 on the flash). */
#include "game/bruce.h"
#include "game/career.h"
#include "game/mess.h"
#include "game/panel.h"
#include "game/psyq.h"

class Sprite {
  public:
	void draw(int, int, int, int, int, int);
};

extern int GVInfo1[];
extern int Cheat_Fire;
extern int GGame;
extern int DAT_800d1798;
extern int GStartTime;
extern int OT_entry;
extern Sprite *pScoreSprite;
extern Sprite *pBarSprite;
extern CBruce *GSkater;
extern int GLevel;
extern volatile unsigned Xblanks;
extern int GHorseLetters[2];
extern SLevel Levels[];

void Horse_DisplayWords(int *);

POLY_G4 *DrawGouraudPoly(int, int, int, int, uchar, uchar, uchar, uchar, uchar, uchar, int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void GraffitiColor(CBruce *);
void Mess_SetScale(int);
int Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

void DisplayScore(CBruce *pBruce, ushort xpos, ushort ypos) {
	int DoDisplay = GVInfo1[0] != 2;

	if (Cheat_Fire) {
		pBruce->mBalanceFlash = 750;
		pBruce->mBalanceTime = 750;
	}

	if (GGame == 7) {
		if (!Cheat_Fire) {
			pBruce->mBalanceFlash = 0;
			pBruce->mBalanceTime = -1;
		}

		Horse_DisplayWords(GHorseLetters);
		return;
	}

	Mess_SetCurrentFont(DAT_800d1798);

	if (DoDisplay) {
		{
			int r;
			int g;
			int b;

			if (pBruce->mBalanceTime != -1) {
				if (pBruce->mBalanceFlash)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->mBalanceFlash * 120 / 750, 3,
					                (Xblanks % 40) * 6, (Xblanks % 40) * 6, 50, (Xblanks % 40) * 6,
					                (Xblanks % 40) * 6, 50, 0);
				else if (pBruce->mBalanceTime)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->mBalanceTime / 25, 3, 0, 150, 0, 0,
					                150, 50, 0);
			}

			OT_entry = 1;
			r = -1;
			g = -1;
			b = -1;

			if (GGame == 8) {
				if (pBruce == GSkater) {
					r = 80;
					g = 80;
					b = 128;
				} else {
					r = 128;
					g = 80;
					b = 80;
				}
			}

			pScoreSprite->draw(xpos, ypos - 20, 1, r, g, b);

			if (GGame != 7) {
				if (pBruce->mBalanceFlash == 0)
					pBarSprite->draw(xpos - 4, ypos, 1, r, g, b);
				else
					pBarSprite->draw(xpos - 4, ypos, 1, (Xblanks % 40) * 6, (Xblanks % 40) * 6, 50);
			}
		}
	}

	if (GGame != 7 && pBruce->mPanel.mLandedTotal && !pBruce->mPanel.m4F4) {
		if (GGame == 8) {
			pBruce->mPanel.mLandedTotal = 0;
		} else if (pBruce->mPanel.mLandedTotal > 10000) {
			pBruce->mPanel.mTotalScore += 250;
			pBruce->mPanel.mLandedTotal -= 250;
		} else if (pBruce->mPanel.mLandedTotal > 2000) {
			pBruce->mPanel.mTotalScore += 100;
			pBruce->mPanel.mLandedTotal -= 100;
		} else if (pBruce->mPanel.mLandedTotal > 25) {
			pBruce->mPanel.mTotalScore += 25;
			pBruce->mPanel.mLandedTotal -= 25;
		} else {
			pBruce->mPanel.mTotalScore += pBruce->mPanel.mLandedTotal;
			pBruce->mPanel.mLandedTotal = 0;
		}
	}

	if (DoDisplay && GGame == 1 && !Levels[GLevel].isCompetition) {
		if (pBruce->mPanel.mTotalScore >= Levels[GLevel].HiScore && !Career_Got(0))
			Panel_Tape(0);

		if (pBruce->mPanel.mTotalScore >= Levels[GLevel].ProScore && !Career_Got(4))
			Panel_Tape(4);
	}

	if (DoDisplay) {
		char score[15];

		if (pBruce->mPanel.mTotalScore > 999999) {
			sprintf(score, "Yowza!", pBruce->mPanel.mTotalScore);
			Mess_SetRGB((Xblanks % 20) * 12, 100, 100, 0);
		} else {
			Mess_SetRGB(128, 128, 128, 0);

			{
				int Value = pBruce->mPanel.mTotalScore;

				if (GGame == 8) {
					Value = pBruce->mGraffitiCount;
					GraffitiColor(pBruce);
				}

				sprintf(score, "%ld", Value);
			}
		}

		Mess_SetTextJustify(2);
		Mess_SetScale(256);
		Mess_DrawPanelText(xpos + 125, ypos - 4, score, 0, 0, 256);
	}

	Mess_SetCurrentFont(GStartTime);
}
