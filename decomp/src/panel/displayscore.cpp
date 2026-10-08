/* DisplayScore__FP6CBruceUsUs (0x8006ca78) */
/* C++ (PANEL.CPP), from the demo twin + its MAIN.SYM locals (DoDisplay; block r/g/b; block score[15]
   with inner Value). Draws the balance meter and score panel, counts ScoreToAdd into Score, awards
   the two score tapes in career mode, and prints the score (flashing above 999999).
   Levers: DrawGouraudPoly returns POLY_G4* and Mess_DrawPanelText returns int (register choice of
   the stacked args); GHorseLetters is a small sized extern ([2]) so its address is one `la` and the
   Horse_DisplayWords jal delay slot stays empty; Sprite::draw takes ints (no &255 on the flash). */
typedef unsigned char uchar;
typedef unsigned short ushort;

class CBruce {
  public:
	char pad0[0x1f0];
	int GraffitiScore; /* 0x1f0 */
	char pad1[0x250 - 0x1f4];
	int BalanceTime;  /* 0x250 */
	int BalanceFlash; /* 0x254 */
	char pad2[0x928 - 0x258];
	unsigned Score; /* 0x928 */
	char pad3[0xa64 - 0x92c];
	unsigned ScoreToAdd; /* 0xa64 */
	char pad4[0xe18 - 0xa68];
	int Bailing; /* 0xe18 */
};

class Sprite {
  public:
	void draw(int, int, int, int, int, int);
};

struct SLevelInfo {
	char pad0[12];
	int NoTapes; /* 0x0c */
	char pad1[0x44 - 0x10];
	unsigned TapeScore1; /* 0x44 */
	unsigned TapeScore2; /* 0x48 */
	char pad2[120 - 0x4c];
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
extern char s_Yowza[];
extern char s_ld_800d1e88[];
extern SLevelInfo Levels[];

void Horse_DisplayWords(int *);

extern "C" {
	void FUN_8001f138(int);
	int Career_Got__Fi(int);
	void Panel_Tape__Fi(int);
	void FUN_8008b900(char *, char *, unsigned);
	void Mess_SetTextJustify__FUc(int);
}

struct POLY_G4;

POLY_G4 *DrawGouraudPoly(int, int, int, int, uchar, uchar, uchar, uchar, uchar, uchar, int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void GraffitiColor(CBruce *);
void Mess_SetScale(int);
int Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

void DisplayScore(CBruce *pBruce, ushort xpos, ushort ypos) {
	int DoDisplay = GVInfo1[0] != 2;

	if (Cheat_Fire) {
		pBruce->BalanceFlash = 750;
		pBruce->BalanceTime = 750;
	}

	if (GGame == 7) {
		if (!Cheat_Fire) {
			pBruce->BalanceFlash = 0;
			pBruce->BalanceTime = -1;
		}

		Horse_DisplayWords(GHorseLetters);
		return;
	}

	FUN_8001f138(DAT_800d1798);

	if (DoDisplay) {
		{
			int r;
			int g;
			int b;

			if (pBruce->BalanceTime != -1) {
				if (pBruce->BalanceFlash)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->BalanceFlash * 120 / 750, 3,
					                (Xblanks % 40) * 6, (Xblanks % 40) * 6, 50,
					                (Xblanks % 40) * 6, (Xblanks % 40) * 6, 50, 0);
				else if (pBruce->BalanceTime)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->BalanceTime / 25, 3, 0, 150, 0, 0,
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
				if (pBruce->BalanceFlash == 0)
					pBarSprite->draw(xpos - 4, ypos, 1, r, g, b);
				else
					pBarSprite->draw(xpos - 4, ypos, 1, (Xblanks % 40) * 6,
					                   (Xblanks % 40) * 6, 50);
			}
		}
	}

	if (GGame != 7 && pBruce->ScoreToAdd && !pBruce->Bailing) {
		if (GGame == 8) {
			pBruce->ScoreToAdd = 0;
		} else if (pBruce->ScoreToAdd > 10000) {
			pBruce->Score += 250;
			pBruce->ScoreToAdd -= 250;
		} else if (pBruce->ScoreToAdd > 2000) {
			pBruce->Score += 100;
			pBruce->ScoreToAdd -= 100;
		} else if (pBruce->ScoreToAdd > 25) {
			pBruce->Score += 25;
			pBruce->ScoreToAdd -= 25;
		} else {
			pBruce->Score += pBruce->ScoreToAdd;
			pBruce->ScoreToAdd = 0;
		}
	}

	if (DoDisplay && GGame == 1 && !Levels[GLevel].NoTapes) {
		if (pBruce->Score >= Levels[GLevel].TapeScore1 && !Career_Got__Fi(0))
			Panel_Tape__Fi(0);

		if (pBruce->Score >= Levels[GLevel].TapeScore2 && !Career_Got__Fi(4))
			Panel_Tape__Fi(4);
	}

	if (DoDisplay) {
		char score[15];

		if (pBruce->Score > 999999) {
			FUN_8008b900(score, s_Yowza, pBruce->Score);
			Mess_SetRGB((Xblanks % 20) * 12, 100, 100, 0);
		} else {
			Mess_SetRGB(128, 128, 128, 0);

			{
				int Value = pBruce->Score;

				if (GGame == 8) {
					Value = pBruce->GraffitiScore;
					GraffitiColor(pBruce);
				}

				FUN_8008b900(score, s_ld_800d1e88, Value);
			}
		}

		Mess_SetTextJustify__FUc(2);
		Mess_SetScale(256);
		Mess_DrawPanelText(xpos + 125, ypos - 4, score, 0, 0, 256);
	}

	FUN_8001f138(GStartTime);
}
