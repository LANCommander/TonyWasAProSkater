/* Front_Display__Fv (0x80017d70) flags=-fstructure-noalias */
/* C++ (FRONT.CPP). Fuzzy demo twin (0.55); locals and statics from its MAIN.SYM. Draws whatever
   the front end is showing, by Status (the current screen): the pause and quit menus, the
   tapes-got screen (creating the spinning tapes the first time round), the yes/no menus, the
   name entry, the H.O.R.S.E. turn screens, the level's high-score table and the end-of-run
   statistics (career totals, two-player result, graffiti tallies or the H.O.R.S.E. letters).
   H.O.R.S.E. case: the 1P stats block has its own `int Line` (the SYM lists two: a function-wide
   s2 one and a block-local s2 one), so the function-wide Line is only ever set to 0. gcse can't
   cprop 0 into the `ashift` of `Line * 24`, so the rows stay live through flow; combine then folds
   them via nonzero_bits (every set of Line is 0), leaving retail's dead `addu s2,zero,zero` in the
   bne delay slot. The NO SCORE row's `Line * 24 + 80` also keeps its call apart from the PUSH
   tail, so cross-jumping shares only `xori; sw` as in retail. */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

class CClass {
  public:
	virtual ~CClass();
};

class CMenu : public CClass {
  public:
	uchar mJustification;
	uchar mRep;
	uchar mLine;
	uchar mHasCursor;
	uchar mCursorLine;
	uchar mNumLines;
	int mX;
	int mY;
	void Display();
	void EntryOn(const char *);
	void EntryOff(const char *);
	void CentreY();
};

class CTape {
  public:
	char pad0[0x140];
	int mSessionTapeNum; /* 0x140 */
	int mTapeNum;        /* 0x144 */
	void AI(int tape_no);
	static int DisplayTitle();
	static int DisplayContinueButton();
};

class CBruce {
  public:
	char pad0[0x1f0];
	int mGraffitiCount; /* 0x1f0 */
	char pad1f4[0x2e8 - 0x1f4];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x8f8 - 0x2ec];
	int mNumTapes; /* 0x8f8 */
	char pad8fc[0x928 - 0x8fc];
	uint mScore; /* 0x928 */
	char pad92c[0x92e - 0x92c];
	ushort mTricksLanded;    /* 0x92e */
	ushort mBails;           /* 0x930 */
	ushort mBestComboTricks; /* 0x932 */
	int mBestComboScore;     /* 0x934 */
	char mBestCombo1[100];   /* 0x938 */
	char mBestCombo2[100];   /* 0x99c */
	char mBestCombo3[100];   /* 0xa00 */
	uint mHorseScore;        /* 0xa64 */
};

struct STrig {
	short s, c;
};

struct SHighScore {
	char Name[4];
	uchar Skater;
	uint Score;
};

struct SCareer {
	char pad0[8];
	uchar LevelTapes[11][14]; /* 8 */
	char pad9e[164 - 8 - 154];
	SHighScore HighScores[23][8]; /* 164 */
	char pad944[0x9bc - 164 - 23 * 96];
	int HorseLength;    /* 0x9bc */
	char HorseWord[16]; /* 0x9c0 */
};

struct SLevelInfo {
	char *pName;
	char pad4[120 - 4];
};

struct SSkaterInfo {
	char *pName;
	char pad[0x154 - 4];
};

extern int Cheat_GameInfo;
extern int TTime;
extern int GNumberOfPlayers;
extern int GGame;
extern int GLevel;
extern int SizeUp;
extern int GHorseCursorPos;
extern int GHorsePlayer;
extern int GHorseMode;
extern int GHorseScore;
extern int GHorseRestart;
extern int GHorseLetters[2];
extern int GHorseWaitTime;
extern int NumHorseRestarts;
extern int GWins1, GLosses1, GStreak1, GHighTags1;
extern int GWins2, GLosses2, GStreak2, GHighTags2;
extern CBruce *GSkater;
extern CBruce *DAT_800d17cc;
extern CBruce *MechList;
extern STrig rcossin_tbl[1];
extern char *GameText[];
extern CTape *GTape[];
extern char GCareer_9c0[];
extern int StatsLeft[];
extern int StatsRight[];
extern SCareer GCareer;
extern SLevelInfo Levels[];
extern SSkaterInfo Stats_2c[];
extern char DAT_800a5b28[];
extern char s_PLAYER_1_SET_SCORE[], s_PLAYER_2_SET_SCORE[], s_PLAYER_1_MATCH[], s_PLAYER_2_MATCH[];
extern char s_TIME_IS_UP[], s_GAME_OVER[], s_ld_of_ld[], s_PLAYER_TWO_WINS[], s_PLAYER_ONE_WINS[];
extern char s_SCORE_SET_AT_d[], s_NO_SCORE_SWITCH[], s_GOING_FOR_d[], s_YOU_SCORED_d[];
extern char s_YOU_ARE_AN[], s_YOU_ARE_A[], s_PLAYER_ONE_d[], s_PLAYER_TWO_d[];
extern char s_d[], DAT_800d14b0[], s_s_800d14b4[], s_ld[];
extern char s_TIE[], s_PUSH[], s_SWITCH[], s_DRAW[];

int Status;
int GamePaused;
int WobbleAmplitude;
int WobbleT;
CMenu *pPausedMenu;
CMenu *pRestartsMenu;
CMenu *pYesNoMenu;
CMenu *pESMenu;
CMenu *pVSMenu;
CMenu *pVS2Menu;
CMenu *pVS3Menu;
CMenu *pEHMenu;
CMenu *pSoundMenu;
int GRollCredits;
int Front_OnTime;
int GHighScoreLineIndex;
int GHighScoreInitialCursorPos;
int GHorseContinue;
int GHorseSounded;
int GHorseScale;
int NextHorseMode;
int NextHorsePlayer;
int NextHorseScore;
int NextHorseRestart;
int NextHorseLetters[2];

void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void Front_MemCardDisplay();
void Front_PrintSelect();
void Front_PrintGoBackSelect();
void Front_PrintCentredPrompt(const char *);
void Horse_DisplayWords(int *);
void Graffiti_Stats(CBruce *, int, int, int, int, int, int, int, int, int);
CTape *Tape_Create(int, int, int, int, int, int, int, int);
int SFX_PlayX(unsigned int, short, int);
int Career_LevelTapes(int);
void Competition_JudgeDisplay();
void Competition_LadderDisplay();
void Competition_WinLoseDisplay();
void Competition_EndGameDisplay();

extern "C" {
	void Panel_DrawFlatShadedPoly__FiiiiUcUcUci(int, int, int, int, uchar, uchar, uchar, int);
	void FUN_800785c4(void);
	void FUN_80017898(void);
	void Mess_SetTextJustify__FUc(int);
	int Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
	int FUN_8008b900(char *, const char *, ...);
	void FUN_8001e07c(char *);
	void FUN_8001e0a4(char *, int);
	void FUN_8001e0f8(char *, short);
	ushort FUN_8001e108(char *);
}

void Front_Display() {
	CBruce *pMan;
	static char Score1[20];
	static char Score2[20];
	static char Tapes[5];
	static char Tricks[8];
	static char Bails[8];
	static char BestCombo[40];
	int Line;

	if (Cheat_GameInfo)
		Panel_DrawFlatShadedPoly__FiiiiUcUcUci(19, 190, 194, 42, 0, 0, 0, 0);

	if (GamePaused)
		FUN_800785c4();

	Mess_SetTextJustify__FUc(0);
	Mess_SetScale(256);

	switch (Status) {
		case 18:
			Front_MemCardDisplay();
			break;
		case 1:
			FUN_80017898();

			if (Cheat_GameInfo)
				Front_PrintSelect();
			else
				Front_PrintGoBackSelect();

			if (WobbleAmplitude) {
				WobbleAmplitude -= 2;
				WobbleT += 690;

				if (WobbleAmplitude < 0) {
					WobbleAmplitude = 0;
					WobbleT = 0;
				}
			}

			pPausedMenu->mX =
			    WobbleAmplitude * (rcossin_tbl + (WobbleT & 4095))->c / 4096 + 256;
			pPausedMenu->Display();
			break;
		case 2:
			if (pRestartsMenu)
				pRestartsMenu->Display();

			break;
		case 3:
			FUN_80017898();
			Front_PrintGoBackSelect();
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText__FiiPCciUiib(256, 95, GameText[111], 0, 0, 256, 0);

			pYesNoMenu->mY = 135;
			pYesNoMenu->Display();
			break;
		case 4:
			FUN_80017898();
			Mess_SetRGB(180, 10, 10, 0);

			if ((TTime / 10) & 1)
				Mess_DrawText__FiiPCciUiib(256, 184, GameText[99], 0, 0, 256, 0);

			break;
		case 11: {
			int NumTapes = GSkater->mNumTapes;

			if (!GTape[0]) {
				SFX_PlayX(19, 8192, 0);

				switch (NumTapes) {
					case 1:
						GTape[0] = Tape_Create(0, 0, 140, 2304, -80, 0, 140, 1024);
						break;
					case 2:
						GTape[0] = Tape_Create(-50, 0, 140, 2176, -80, -25, 140, 1024);
						GTape[1] = Tape_Create(50, 0, 140, 2176, -80, 25, 140, 1024);
						break;
					case 3:
						GTape[0] = Tape_Create(0, 0, 140, 2304, -80, -40, 140, 1024);
						GTape[1] = Tape_Create(-60, 0, 140, 1792, -80, 0, 140, 1024);
						GTape[2] = Tape_Create(60, 0, 140, 1792, -80, 40, 140, 1024);
						break;
					case 4:
						GTape[0] = Tape_Create(-45, -25, 140, 1792, -80, -50, 140, 1024);
						GTape[1] = Tape_Create(45, -25, 140, 1792, -70, -20, 140, 1024);
						GTape[2] = Tape_Create(-45, 25, 140, 1792, -80, 10, 140, 1024);
						GTape[3] = Tape_Create(45, 25, 140, 1792, -70, 40, 140, 1024);
						break;
					case 5:
						GTape[0] = Tape_Create(0, 0, 140, 2304, -80, -50, 140, 1024);
						GTape[1] = Tape_Create(-55, -25, 140, 1536, -70, -25, 140, 1024);
						GTape[2] = Tape_Create(55, -25, 140, 1536, -80, 0, 140, 1024);
						GTape[3] = Tape_Create(-55, 25, 140, 1536, -70, 25, 140, 1024);
						GTape[4] = Tape_Create(55, 25, 140, 1536, -80, 50, 140, 1024);
						break;
				}
			}

			{
				int i;

				for (i = 0; i < 5; i++) {
					if (GTape[i]) {
						GTape[i]->mTapeNum =
						    GCareer.LevelTapes[GSkater->mSkaterIndex][0] - (NumTapes - i);
						GTape[i]->mSessionTapeNum = i;
					}
				}
			}

			{
				int i;

				for (i = 0; i < 5; i++) {
					if (GTape[i])
						GTape[i]->AI(GCareer.LevelTapes[GSkater->mSkaterIndex][0] -
						                    (NumTapes - i));
				}
			}

			Mess_SetScale(256);
			Mess_SetRGB(122, 122, 122, 0);

			if (CTape::DisplayTitle()) {
				static char GotATape[25];

				if (NumTapes == 1) {
					Mess_DrawText__FiiPCciUiib(256, 40, GameText[35], 0, 0, 256, 0);
				} else {
					FUN_8008b900(GotATape, GameText[36], NumTapes);
					Mess_DrawText__FiiPCciUiib(256, 40, GotATape, 0, 0, 256, 0);
				}
			}

			if (CTape::DisplayContinueButton())
				Front_PrintCentredPrompt(GameText[96]);
		} break;
		case 6:
		case 7:
		case 12:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText__FiiPCciUiib(256, 80, GameText[5], 0, 0, 256, 0);

			pYesNoMenu->mY = 130;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText[102]);
			break;
		case 17:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText__FiiPCciUiib(256, 80, GameText[47], 0, 0, 256, 0);

			pYesNoMenu->mY = 130;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText[102]);
			break;
		case 14:
			if (!GRollCredits && Front_OnTime >= 11) {
				pESMenu->Display();
				Front_PrintCentredPrompt(GameText[102]);
			}

			break;
		case 15:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVSMenu->EntryOff(GameText[24]);
				else
					pVSMenu->EntryOn(GameText[24]);

				pVSMenu->CentreY();
				pVSMenu->Display();
				Front_PrintGoBackSelect();
			}

			break;
		case 16:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVS2Menu->EntryOff(GameText[24]);
				else
					pVS2Menu->EntryOn(GameText[24]);

				pVS2Menu->CentreY();
				pVS2Menu->Display();
				Front_PrintGoBackSelect();
			}

			break;
		case 34: {
			char horseBuf[2];
			int termFound = 0;
			int i;

			Mess_SetTextJustify__FUc(0);
			Mess_SetRGB(122, 122, 122, 0);
			Mess_DrawText__FiiPCciUiib(256, 80, GameText[81], 0, 0, 256, 0);

			horseBuf[1] = 0;

			for (i = 0; i < 10; i++) {
				char *pWord = GCareer_9c0;

				if (SizeUp > 0)
					SizeUp--;

				if (!pWord[i])
					termFound = 1;

				horseBuf[0] = pWord[i];

				if (i == GHorseCursorPos) {
					Mess_SetRGB(182, 132, 36, 0);
					Mess_SetScale(SizeUp * 6 + 256);
				} else {
					Mess_SetRGB(122, 122, 122, 0);
					Mess_SetScale(256);
				}

				if (!termFound)
					Mess_DrawText__FiiPCciUiib(i * 20 + 156, 120, horseBuf, 0, 0, 256, 0);

				horseBuf[0] = '_';
				Mess_SetScale(256);
				Mess_DrawText__FiiPCciUiib(i * 20 + 156, 130, horseBuf, 0, 0, 256, 0);
			}

			Front_PrintCentredPrompt(GameText[103]);
		} break;
		case 33: {
			char Score[10];

			Mess_SetScale(256);

			if (GHorsePlayer == 0)
				Mess_SetRGB(80, 80, 128, 0);
			else
				Mess_SetRGB(128, 80, 80, 0);

			if (GHorseMode == 0) {
				if (GHorsePlayer == 0)
					Mess_DrawText__FiiPCciUiib(256, 120, s_PLAYER_1_SET_SCORE, 0, 0, 256, 0);
				else
					Mess_DrawText__FiiPCciUiib(256, 120, s_PLAYER_2_SET_SCORE, 0, 0, 256, 0);
			} else {
				if (GHorsePlayer == 0)
					Mess_DrawText__FiiPCciUiib(256, 120, s_PLAYER_1_MATCH, 0, 0, 256, 0);
				else
					Mess_DrawText__FiiPCciUiib(256, 120, s_PLAYER_2_MATCH, 0, 0, 256, 0);

				FUN_8008b900(Score, s_d, GHorseScore);
				Mess_DrawText__FiiPCciUiib(256, 140, Score, 0, 0, 256, 0);
			}

			Horse_DisplayWords(GHorseLetters);
			Front_PrintCentredPrompt(GameText[96]);
		} break;
		case 37: {
			static char highScoreBuf[64];
			int i;

			Mess_SetScale(256);
			FUN_8008b900(highScoreBuf, GameText[80], Levels[GLevel].pName);
			Mess_SetRGB(182, 132, 36, 0);
			Mess_DrawText__FiiPCciUiib(256, 40, highScoreBuf, 0, 0, 256, 0);
			Mess_SetRGB(122, 122, 122, 0);

			for (i = 0; i < 8; i++) {
				StatsLeft[i] += 30;

				if (StatsLeft[i] > 180)
					StatsLeft[i] = 180;

				StatsRight[i] -= 30;

				if (StatsRight[i] < 412)
					StatsRight[i] = 412;

				if (i == GHighScoreLineIndex)
					Mess_SetRGB(182, 132, 36, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				if (SizeUp > 0)
					SizeUp--;

				highScoreBuf[1] = 0;
				highScoreBuf[0] = GCareer.HighScores[GLevel][i].Name[0];
				Mess_SetTextJustify__FUc(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 0)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText__FiiPCciUiib(StatsLeft[i] - 80, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = GCareer.HighScores[GLevel][i].Name[1];
				Mess_SetTextJustify__FUc(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 1)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText__FiiPCciUiib(StatsLeft[i] - 60, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = GCareer.HighScores[GLevel][i].Name[2];
				Mess_SetTextJustify__FUc(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 2)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText__FiiPCciUiib(StatsLeft[i] - 40, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				if (i == GHighScoreLineIndex && (TTime / 10) & 1) {
					int x;

					FUN_8008b900(highScoreBuf, DAT_800d14b0);
					Mess_SetTextJustify__FUc(0);

					x = GHighScoreInitialCursorPos * 20 - 80;
					Mess_DrawText__FiiPCciUiib(StatsLeft[i] + x, (i + 1) * 19 + 48, highScoreBuf, 0, 0, 256,
					             0);
				}

				FUN_8008b900(highScoreBuf, s_s_800d14b4,
				             Stats_2c[GCareer.HighScores[GLevel][i].Skater].pName);

				{
					char *p = highScoreBuf;

					while (*p) {
						if (*p == ' ') {
							*p = 0;
							break;
						}

						p++;
					}
				}

				Mess_SetTextJustify__FUc(1);
				Mess_DrawText__FiiPCciUiib(StatsLeft[i], (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				FUN_8008b900(highScoreBuf, s_d,
				             GCareer.HighScores[GLevel][i].Score);
				Mess_SetTextJustify__FUc(2);
				Mess_DrawText__FiiPCciUiib(StatsRight[i], (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
			}

			Front_PrintCentredPrompt(GameText[103]);
		} break;
		case 13:
			Mess_SetScale(256);

			if (GNumberOfPlayers == 1) {
				Mess_SetRGB(182, 132, 36, 0);
				Mess_DrawText__FiiPCciUiib(256, 40, GameText[37], 0, 0, 256, 0);
			} else {
				Mess_SetRGB(182, 132, 36, 0);

				if (GGame == 7)
					Mess_DrawText__FiiPCciUiib(256, 40, s_TIME_IS_UP, 0, 0, 256, 0);
				else
					Mess_DrawText__FiiPCciUiib(256, 40, s_GAME_OVER, 0, 0, 256, 0);
			}

			Mess_SetRGB(122, 122, 122, 0);
			Mess_SetScale(256);

			if (GNumberOfPlayers == 1) {
				int Line;
				int i;

				Mess_SetTextJustify__FUc(1);

				for (i = 0; i < 5; i++) {
					StatsLeft[i] += 30;

					if (StatsLeft[i] > 90)
						StatsLeft[i] = 90;
				}

				Line = 0;
				Mess_DrawText__FiiPCciUiib(StatsLeft[0], Line++ * 18 + 60, GameText[38], 0, 0, 256, 0);
				Mess_DrawText__FiiPCciUiib(StatsLeft[1], Line++ * 18 + 60, GameText[39], 0, 0, 256, 0);

				if (GGame == 1) {
					Mess_DrawText__FiiPCciUiib(StatsLeft[2], Line++ * 18 + 60, GameText[40], 0, 0, 256, 0);
				}

				Mess_DrawText__FiiPCciUiib(StatsLeft[3], Line++ * 18 + 60, GameText[41], 0, 0, 256, 0);
				Mess_DrawText__FiiPCciUiib(StatsLeft[4], Line * 18 + 60, GameText[42], 0, 0, 256, 0);
				Mess_SetTextJustify__FUc(0);

				pMan = MechList;
				Mess_SetTextJustify__FUc(2);

				for (i = 0; i < 5; i++) {
					StatsRight[i] -= 30;

					if (StatsRight[i] < 410)
						StatsRight[i] = 410;
				}

				Line = 0;
				FUN_8008b900(Score1, s_d, pMan->mScore);
				Mess_DrawText__FiiPCciUiib(StatsRight[0], Line++ * 18 + 60, Score1, 0, 0, 256, 0);
				FUN_8008b900(Score2, s_d, GCareer.HighScores[GLevel][0].Score);
				Mess_DrawText__FiiPCciUiib(StatsRight[1], Line++ * 18 + 60, Score2, 0, 0, 256, 0);

				if (GGame == 1) {
					FUN_8008b900(Tapes, s_ld_of_ld, Career_LevelTapes(GLevel), 5);
					Mess_DrawText__FiiPCciUiib(StatsRight[2], Line++ * 18 + 60, Tapes, 0, 0, 256, 0);
				}

				FUN_8008b900(Tricks, s_ld, pMan->mTricksLanded);
				Mess_DrawText__FiiPCciUiib(StatsRight[3], Line++ * 18 + 60, Tricks, 0, 0, 256, 0);
				FUN_8008b900(Bails, s_ld, pMan->mBails);
				Mess_DrawText__FiiPCciUiib(StatsRight[4], Line * 18 + 60, Bails, 0, 0, 256, 0);

				if (StatsLeft[0] == 90) {
					Mess_SetTextJustify__FUc(0);

					if (!pMan->mBestComboScore) {
						Mess_SetScale(220);
						Mess_DrawText__FiiPCciUiib(256, 151, GameText[43], 0, 0, 256, 0);
						Mess_SetScale(256);
						Mess_DrawText__FiiPCciUiib(256, 168, GameText[44], 0, 0, 256, 0);
					} else {
						if (pMan->mBestComboTricks == 1)
							FUN_8008b900(BestCombo, GameText[46], pMan->mBestComboTricks,
							             pMan->mBestComboScore);
						else
							FUN_8008b900(BestCombo, GameText[45], pMan->mBestComboTricks,
							             pMan->mBestComboScore);

						Mess_SetScale(220);
						Mess_DrawText__FiiPCciUiib(256, 151, BestCombo, 0, 0, 256, 0);
						Mess_SetScale(188);
						Mess_DrawText__FiiPCciUiib(256, 166, pMan->mBestCombo1, 0, 0, 256, 0);
						Mess_DrawText__FiiPCciUiib(256, 180, pMan->mBestCombo2, 0, 0, 256, 0);
						Mess_DrawText__FiiPCciUiib(256, 194, pMan->mBestCombo3, 0, 0, 256, 0);
					}
				}
			} else {
				int s1, s2;

				Mess_SetScale(256);

				s1 = GSkater->mScore;
				s2 = 0;

				if (DAT_800d17cc)
					s2 = DAT_800d17cc->mScore;

				switch (GGame) {
					case 8:
						if (DAT_800d17cc->mGraffitiCount == GSkater->mGraffitiCount)
							Mess_DrawText__FiiPCciUiib(256, 64, s_TIE, 0, 0, 256, 0);
						else if (GSkater->mGraffitiCount < DAT_800d17cc->mGraffitiCount)
							Mess_DrawText__FiiPCciUiib(256, 64, s_PLAYER_TWO_WINS, 0, 0, 256, 0);
						else
							Mess_DrawText__FiiPCciUiib(256, 64, s_PLAYER_ONE_WINS, 0, 0, 256, 0);

						Mess_SetScale(224);
						Graffiti_Stats(GSkater, 30, 86, 32, 32, 128, GWins1,
						               GLosses1, GStreak1, GHighTags1);
						Graffiti_Stats(DAT_800d17cc, 286, 86, 128, 32, 32, GWins2,
						               GLosses2, GStreak2, GHighTags2);
						break;
					case 7: {
						char Score[40];

						NextHorseMode = GHorseMode;
						NextHorsePlayer = GHorsePlayer;
						NextHorseScore = GHorseScore;
						NextHorseRestart = GHorseRestart;
						NextHorseLetters[0] = GHorseLetters[0];
						NextHorseLetters[1] = GHorseLetters[1];
						Line = 0;

						if (GHorsePlayer == 0)
							Mess_SetRGB(80, 80, 128, 0);
						else
							Mess_SetRGB(128, 80, 80, 0);

						if (GHorseMode == 0) {
							GHorseScore = GSkater->mHorseScore;

							if (GHorseScore) {
								FUN_8008b900(Score, s_SCORE_SET_AT_d, GHorseScore);
								Mess_DrawText__FiiPCciUiib(256, Line * 24 + 80, Score, 0, 0, 256, 0);

								NextHorseMode ^= 1;
								NextHorsePlayer ^= 1;
							} else {
								Mess_DrawText__FiiPCciUiib(256, Line * 24 + 80, s_NO_SCORE_SWITCH, 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
							}
						} else {
							FUN_8008b900(Score, s_GOING_FOR_d, GHorseScore);
							Mess_DrawText__FiiPCciUiib(256, Line++ * 24 + 80, Score, 0, 0, 256, 0);
							FUN_8008b900(Score, s_YOU_SCORED_d, GSkater->mHorseScore);
							Mess_DrawText__FiiPCciUiib(256, Line++ * 24 + 80, Score, 0, 0, 256, 0);

							if (GSkater->mHorseScore == (uint)GHorseScore) {
								Mess_DrawText__FiiPCciUiib(256, Line * 24 + 80, s_PUSH, 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
							} else if (GSkater->mHorseScore < (uint)GHorseScore) {
								char YouAre[40];
								int Letters = NextHorseLetters[GHorsePlayer]++;

								if (Letters + 1 == GCareer.HorseLength) {
									char c = GCareer.HorseWord[0];

									if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U')
										FUN_8008b900(YouAre, s_YOU_ARE_AN);
									else
										FUN_8008b900(YouAre, s_YOU_ARE_A);

									if (!GHorseContinue) {
										GHorseScale += 32;

										if (GHorseScale > 512) {
											Mess_SetRGB(255, 48, 48, 0);

											GHorseScale = 512;
										}

										Mess_SetScale(GHorseScale);
										Mess_DrawText__FiiPCciUiib(256, 152, YouAre, 0, 0, 256, 0);
										Mess_SetScale(GHorseScale <= 512 ? GHorseScale : 512);
										Mess_DrawText__FiiPCciUiib(256, 188, GCareer_9c0, 0, 0, 256, 0);

										GHorseWaitTime = 45;
										Front_PrintCentredPrompt(GameText[96]);
									} else {
										if (!GHorseSounded) {
											SFX_PlayX(42, 8192, 0);

											GHorseSounded = 1;
										}

										FUN_8001e07c(DAT_800a5b28);
										FUN_8001e0f8(DAT_800a5b28, 512);
										FUN_8001e0a4(DAT_800a5b28, 1);
										Mess_DrawText__FiiPCciUiib(256, 152, YouAre, 0, 0, 256, 0);

										if (FUN_8001e108(DAT_800a5b28))
											GHorseWaitTime = 45;

										FUN_8001e0a4(DAT_800a5b28, 0);
										FUN_8001e0a4(DAT_800a5b28, 1);
										Mess_DrawText__FiiPCciUiib(256, 188, GCareer_9c0, 0, 0, 256, 0);
										FUN_8001e0a4(DAT_800a5b28, 0);
									}

									NextHorseMode = 2;
								} else {
									YouAre[1] = 0;
									YouAre[0] = GCareer.HorseWord[Letters];

									if (GHorseScale < 512) {
										GHorseScale += 32;

										if (GHorseScale > 512)
											GHorseScale = 512;

										Mess_SetScale(GHorseScale <= 384 ? GHorseScale : 384);
										Mess_DrawText__FiiPCciUiib(256, 152, GameText[88], 0, 0, 256, 0);
										Mess_SetScale(GHorseScale <= 512 ? GHorseScale : 512);
										Mess_DrawText__FiiPCciUiib(256, 188, YouAre, 0, 0, 256, 0);

										GHorseWaitTime = 45;
									} else {
										if (!GHorseSounded) {
											SFX_PlayX(42, 8192, 0);

											GHorseSounded = 1;
										}

										FUN_8001e07c(DAT_800a5b28);
										FUN_8001e0f8(DAT_800a5b28, 384);
										FUN_8001e0a4(DAT_800a5b28, 1);
										Mess_DrawText__FiiPCciUiib(256, 152, GameText[88], 0, 0, 256, 0);

										if (FUN_8001e108(DAT_800a5b28))
											GHorseWaitTime = 45;

										FUN_8001e0a4(DAT_800a5b28, 0);
										FUN_8001e0f8(DAT_800a5b28, 512);
										FUN_8001e0a4(DAT_800a5b28, 1);
										Mess_DrawText__FiiPCciUiib(256, 188, YouAre, 0, 0, 256, 0);
										FUN_8001e0a4(DAT_800a5b28, 0);
									}

									if (++NextHorseRestart == NumHorseRestarts)
										NextHorseRestart = 0;

									NextHorsePlayer ^= 1;
									NextHorseMode ^= 1;
								}
							} else {
								Mess_DrawText__FiiPCciUiib(256, Line * 24 + 80, s_SWITCH, 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
								NextHorseScore = GSkater->mHorseScore;
							}
						}

						Horse_DisplayWords(NextHorseLetters);
					} break;
					case 6:
					default:
						FUN_8008b900(Score1, s_PLAYER_ONE_d, s1);
						FUN_8008b900(Score2, s_PLAYER_TWO_d, s2);
						Mess_DrawText__FiiPCciUiib(256, 75, Score1, 0, 0, 256, 0);
						Mess_DrawText__FiiPCciUiib(256, 99, Score2, 0, 0, 256, 0);

						if (s2 < s1)
							Mess_DrawText__FiiPCciUiib(256, 123, s_PLAYER_ONE_WINS, 0, 0, 256, 0);
						else if (s1 < s2)
							Mess_DrawText__FiiPCciUiib(256, 123, s_PLAYER_TWO_WINS, 0, 0, 256, 0);
						else
							Mess_DrawText__FiiPCciUiib(256, 123, s_DRAW, 0, 0, 256, 0);

						break;
				}
			}

			if (GGame != 7 && Front_OnTime >= 11)
				Front_PrintCentredPrompt(GameText[96]);

			break;
		case 5:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText__FiiPCciUiib(256, 103, GameText[97], 0, 0, 256, 0);

			pYesNoMenu->mY = 143;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText[102]);
			break;
		case 24:
			if (Front_OnTime >= 11) {
				pEHMenu->Display();
				Front_PrintCentredPrompt(GameText[102]);
			}

			break;
		case 25:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVS3Menu->EntryOff(GameText[24]);
				else
					pVS3Menu->EntryOn(GameText[24]);

				pVS3Menu->CentreY();
				pVS3Menu->Display();
				Front_PrintGoBackSelect();
			}

			break;
		case 27:
			Competition_EndGameDisplay();
			break;
		case 23:
			Competition_WinLoseDisplay();
			break;
		case 22:
			Competition_LadderDisplay();
			break;
		case 20:
			Competition_JudgeDisplay();
			break;
		case 38:
			pSoundMenu->Display();
			Front_PrintCentredPrompt(GameText[102]);
			break;
	}
}
