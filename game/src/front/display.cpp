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

#include "game/front.h"
#include "game/tapes.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/mess.h"
#include "game/panel.h"
#include "game/psyq.h"
#include "game/shell.h"

struct STrig {
	short s, c;
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
extern LocalText GameText;
extern CTape *GTape[];
extern int StatsLeft[];
extern int StatsRight[];
extern SCareer GCareer;
extern SLevel Levels[];
extern SStats Stats[];
extern FallingTextEffect DAT_800a5b28[]; /* GHorseFallingText */

extern int Status;
extern int GamePaused;
extern int WobbleAmplitude;
extern int WobbleT;
extern CMenu *pPausedMenu;
extern CMenu *pRestartsMenu;
extern CMenu *pYesNoMenu;
extern CMenu *pESMenu;
extern CMenu *pVSMenu;
extern CMenu *pVS2Menu;
extern CMenu *pVS3Menu;
extern CMenu *pEHMenu;
extern CMenu *pSoundMenu;
extern int GRollCredits;
extern int Front_OnTime;
extern int GHighScoreLineIndex;
extern int GHighScoreInitialCursorPos;
extern int GHorseContinue;
extern int GHorseSounded;
extern int GHorseScale;
extern int NextHorseMode;
extern int NextHorsePlayer;
extern int NextHorseScore;
extern int NextHorseRestart;
extern int NextHorseLetters[2];

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
		Panel_DrawFlatShadedPoly(19, 190, 194, 42, 0, 0, 0, 0);

	if (GamePaused)
		Shell_PauseScreen();

	Mess_SetTextJustify(0);
	Mess_SetScale(256);

	switch (Status) {
		case 18:
			Front_MemCardDisplay();
			break;
		case 1:
			PrintPaused();

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

			pPausedMenu->mX = WobbleAmplitude * (rcossin_tbl + (WobbleT & 4095))->c / 4096 + 256;
			pPausedMenu->Display();
			break;
		case 2:
			if (pRestartsMenu)
				pRestartsMenu->Display();

			break;
		case 3:
			PrintPaused();
			Front_PrintGoBackSelect();
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 95, GameText.pReallyQuit, 0, 0, 256, 0);

			pYesNoMenu->mY = 135;
			pYesNoMenu->Display();
			break;
		case 4:
			PrintPaused();
			Mess_SetRGB(180, 10, 10, 0);

			if ((TTime / 10) & 1)
				Mess_DrawText(256, 184, GameText.pPleaseReinsertController, 0, 0, 256, 0);

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
						    GCareer.CharacterProgress[GSkater->mSkaterIndex].NumTapes -
						    (NumTapes - i);
						GTape[i]->mSessionTapeNum = i;
					}
				}
			}

			{
				int i;

				for (i = 0; i < 5; i++) {
					if (GTape[i])
						GTape[i]->AI(GCareer.CharacterProgress[GSkater->mSkaterIndex].NumTapes -
						             (NumTapes - i));
				}
			}

			Mess_SetScale(256);
			Mess_SetRGB(122, 122, 122, 0);

			if (CTape::DisplayTitle()) {
				static char GotATape[25];

				if (NumTapes == 1) {
					Mess_DrawText(256, 40, GameText.pYouGotATape, 0, 0, 256, 0);
				} else {
					sprintf(GotATape, GameText.pYouGotNTapes, NumTapes);
					Mess_DrawText(256, 40, GotATape, 0, 0, 256, 0);
				}
			}

			if (CTape::DisplayContinueButton())
				Front_PrintCentredPrompt(GameText.pContinue);
		} break;
		case 6:
		case 7:
		case 12:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 80, GameText.pSaveCareerQM, 0, 0, 256, 0);

			pYesNoMenu->mY = 130;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText.pToSelect);
			break;
		case 17:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 80, GameText.pAreYouSure, 0, 0, 256, 0);

			pYesNoMenu->mY = 130;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText.pToSelect);
			break;
		case 14:
			if (!GRollCredits && Front_OnTime >= 11) {
				pESMenu->Display();
				Front_PrintCentredPrompt(GameText.pToSelect);
			}

			break;
		case 15:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVSMenu->EntryOff(GameText.pHighlights);
				else
					pVSMenu->EntryOn(GameText.pHighlights);

				pVSMenu->CentreY();
				pVSMenu->Display();
				Front_PrintGoBackSelect();
			}

			break;
		case 16:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVS2Menu->EntryOff(GameText.pHighlights);
				else
					pVS2Menu->EntryOn(GameText.pHighlights);

				pVS2Menu->CentreY();
				pVS2Menu->Display();
				Front_PrintGoBackSelect();
			}

			break;
		case 34: {
			char horseBuf[2];
			int termFound = 0;
			int i;

			Mess_SetTextJustify(0);
			Mess_SetRGB(122, 122, 122, 0);
			Mess_DrawText(256, 80, GameText.pHorsePlayTo, 0, 0, 256, 0);

			horseBuf[1] = 0;

			for (i = 0; i < 10; i++) {
				char *pWord = GCareer.Options.HorseWord;

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
					Mess_DrawText(i * 20 + 156, 120, horseBuf, 0, 0, 256, 0);

				horseBuf[0] = '_';
				Mess_SetScale(256);
				Mess_DrawText(i * 20 + 156, 130, horseBuf, 0, 0, 256, 0);
			}

			Front_PrintCentredPrompt(GameText.pToAccept);
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
					Mess_DrawText(256, 120, "PLAYER 1 SET SCORE", 0, 0, 256, 0);
				else
					Mess_DrawText(256, 120, "PLAYER 2 SET SCORE", 0, 0, 256, 0);
			} else {
				if (GHorsePlayer == 0)
					Mess_DrawText(256, 120, "PLAYER 1 MATCH", 0, 0, 256, 0);
				else
					Mess_DrawText(256, 120, "PLAYER 2 MATCH", 0, 0, 256, 0);

				sprintf(Score, "%d", GHorseScore);
				Mess_DrawText(256, 140, Score, 0, 0, 256, 0);
			}

			Horse_DisplayWords(GHorseLetters);
			Front_PrintCentredPrompt(GameText.pContinue);
		} break;
		case 37: {
			static char highScoreBuf[64];
			int i;

			Mess_SetScale(256);
			sprintf(highScoreBuf, GameText.pHighScores, Levels[GLevel].pLevelName);
			Mess_SetRGB(182, 132, 36, 0);
			Mess_DrawText(256, 40, highScoreBuf, 0, 0, 256, 0);
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
				highScoreBuf[0] = GCareer.HighScore[GLevel][i].Name[0];
				Mess_SetTextJustify(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 0)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText(StatsLeft[i] - 80, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = GCareer.HighScore[GLevel][i].Name[1];
				Mess_SetTextJustify(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 1)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText(StatsLeft[i] - 60, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = GCareer.HighScore[GLevel][i].Name[2];
				Mess_SetTextJustify(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 2)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText(StatsLeft[i] - 40, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				if (i == GHighScoreLineIndex && (TTime / 10) & 1) {
					int x;

					sprintf(highScoreBuf, "_");
					Mess_SetTextJustify(0);

					x = GHighScoreInitialCursorPos * 20 - 80;
					Mess_DrawText(StatsLeft[i] + x, (i + 1) * 19 + 48, highScoreBuf, 0, 0, 256, 0);
				}

				sprintf(highScoreBuf, "%s", Stats[GCareer.HighScore[GLevel][i].Skater].Name);

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

				Mess_SetTextJustify(1);
				Mess_DrawText(StatsLeft[i], (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				sprintf(highScoreBuf, "%d", GCareer.HighScore[GLevel][i].Score);
				Mess_SetTextJustify(2);
				Mess_DrawText(StatsRight[i], (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
			}

			Front_PrintCentredPrompt(GameText.pToAccept);
		} break;
		case 13:
			Mess_SetScale(256);

			if (GNumberOfPlayers == 1) {
				Mess_SetRGB(182, 132, 36, 0);
				Mess_DrawText(256, 40, GameText.pStatistics, 0, 0, 256, 0);
			} else {
				Mess_SetRGB(182, 132, 36, 0);

				if (GGame == 7)
					Mess_DrawText(256, 40, "TIME IS UP!", 0, 0, 256, 0);
				else
					Mess_DrawText(256, 40, "GAME OVER", 0, 0, 256, 0);
			}

			Mess_SetRGB(122, 122, 122, 0);
			Mess_SetScale(256);

			if (GNumberOfPlayers == 1) {
				int Line;
				int i;

				Mess_SetTextJustify(1);

				for (i = 0; i < 5; i++) {
					StatsLeft[i] += 30;

					if (StatsLeft[i] > 90)
						StatsLeft[i] = 90;
				}

				Line = 0;
				Mess_DrawText(StatsLeft[0], Line++ * 18 + 60, GameText.pTotalScore, 0, 0, 256, 0);
				Mess_DrawText(StatsLeft[1], Line++ * 18 + 60, GameText.pHighScore, 0, 0, 256, 0);

				if (GGame == 1) {
					Mess_DrawText(StatsLeft[2], Line++ * 18 + 60, GameText.pTapesCollected, 0, 0,
					              256, 0);
				}

				Mess_DrawText(StatsLeft[3], Line++ * 18 + 60, GameText.pTricksLanded, 0, 0, 256, 0);
				Mess_DrawText(StatsLeft[4], Line * 18 + 60, GameText.pBails, 0, 0, 256, 0);
				Mess_SetTextJustify(0);

				pMan = MechList;
				Mess_SetTextJustify(2);

				for (i = 0; i < 5; i++) {
					StatsRight[i] -= 30;

					if (StatsRight[i] < 410)
						StatsRight[i] = 410;
				}

				Line = 0;
				sprintf(Score1, "%d", pMan->mPanel.mTotalScore);
				Mess_DrawText(StatsRight[0], Line++ * 18 + 60, Score1, 0, 0, 256, 0);
				sprintf(Score2, "%d", GCareer.HighScore[GLevel][0].Score);
				Mess_DrawText(StatsRight[1], Line++ * 18 + 60, Score2, 0, 0, 256, 0);

				if (GGame == 1) {
					sprintf(Tapes, "%ld of %ld", (long)Career_LevelTapes(GLevel), 5L);
					Mess_DrawText(StatsRight[2], Line++ * 18 + 60, Tapes, 0, 0, 256, 0);
				}

				sprintf(Tricks, "%ld", (long)pMan->mPanel.mNumTricksLanded);
				Mess_DrawText(StatsRight[3], Line++ * 18 + 60, Tricks, 0, 0, 256, 0);
				sprintf(Bails, "%ld", (long)pMan->mPanel.mNumBails);
				Mess_DrawText(StatsRight[4], Line * 18 + 60, Bails, 0, 0, 256, 0);

				if (StatsLeft[0] == 90) {
					Mess_SetTextJustify(0);

					if (!pMan->mPanel.mBestComboScore) {
						Mess_SetScale(220);
						Mess_DrawText(256, 151, GameText.pBestCombo, 0, 0, 256, 0);
						Mess_SetScale(256);
						Mess_DrawText(256, 168, GameText.pNone, 0, 0, 256, 0);
					} else {
						if (pMan->mPanel.mNumTricksInCombo == 1)
							sprintf(BestCombo, GameText.pBestComboNTrickNPoints,
							        pMan->mPanel.mNumTricksInCombo, pMan->mPanel.mBestComboScore);
						else
							sprintf(BestCombo, GameText.pBestComboNTricksNPoints,
							        pMan->mPanel.mNumTricksInCombo, pMan->mPanel.mBestComboScore);

						Mess_SetScale(220);
						Mess_DrawText(256, 151, BestCombo, 0, 0, 256, 0);
						Mess_SetScale(188);
						Mess_DrawText(256, 166, pMan->mPanel.mBestComboName, 0, 0, 256, 0);
						Mess_DrawText(256, 180, pMan->mPanel.mBestComboName2, 0, 0, 256, 0);
						Mess_DrawText(256, 194, pMan->mPanel.mBestComboName3, 0, 0, 256, 0);
					}
				}
			} else {
				int s1, s2;

				Mess_SetScale(256);

				s1 = GSkater->mPanel.mTotalScore;
				s2 = 0;

				if (DAT_800d17cc)
					s2 = DAT_800d17cc->mPanel.mTotalScore;

				switch (GGame) {
					case 8:
						if (DAT_800d17cc->mGraffitiCount == GSkater->mGraffitiCount)
							Mess_DrawText(256, 64, "TIE", 0, 0, 256, 0);
						else if (GSkater->mGraffitiCount < DAT_800d17cc->mGraffitiCount)
							Mess_DrawText(256, 64, "PLAYER TWO WINS", 0, 0, 256, 0);
						else
							Mess_DrawText(256, 64, "PLAYER ONE WINS", 0, 0, 256, 0);

						Mess_SetScale(224);
						Graffiti_Stats(GSkater, 30, 86, 32, 32, 128, GWins1, GLosses1, GStreak1,
						               GHighTags1);
						Graffiti_Stats(DAT_800d17cc, 286, 86, 128, 32, 32, GWins2, GLosses2,
						               GStreak2, GHighTags2);
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
							GHorseScore = GSkater->mPanel.mLandedTotal;

							if (GHorseScore) {
								sprintf(Score, "SCORE SET AT %d", GHorseScore);
								Mess_DrawText(256, Line * 24 + 80, Score, 0, 0, 256, 0);

								NextHorseMode ^= 1;
								NextHorsePlayer ^= 1;
							} else {
								Mess_DrawText(256, Line * 24 + 80, "NO SCORE   SWITCH", 0, 0, 256,
								              0);

								NextHorsePlayer ^= 1;
							}
						} else {
							sprintf(Score, "GOING FOR %d", GHorseScore);
							Mess_DrawText(256, Line++ * 24 + 80, Score, 0, 0, 256, 0);
							sprintf(Score, "YOU SCORED %d", GSkater->mPanel.mLandedTotal);
							Mess_DrawText(256, Line++ * 24 + 80, Score, 0, 0, 256, 0);

							if (GSkater->mPanel.mLandedTotal == (uint)GHorseScore) {
								Mess_DrawText(256, Line * 24 + 80, "PUSH", 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
							} else if (GSkater->mPanel.mLandedTotal < (uint)GHorseScore) {
								char YouAre[40];
								int Letters = NextHorseLetters[GHorsePlayer]++;

								if (Letters + 1 == GCareer.Options.HorseLen) {
									char c = GCareer.Options.HorseWord[0];

									if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U')
										sprintf(YouAre, "YOU ARE AN");
									else
										sprintf(YouAre, "YOU ARE A");

									if (!GHorseContinue) {
										GHorseScale += 32;

										if (GHorseScale > 512) {
											Mess_SetRGB(255, 48, 48, 0);

											GHorseScale = 512;
										}

										Mess_SetScale(GHorseScale);
										Mess_DrawText(256, 152, YouAre, 0, 0, 256, 0);
										Mess_SetScale(GHorseScale <= 512 ? GHorseScale : 512);
										Mess_DrawText(256, 188, GCareer.Options.HorseWord, 0, 0,
										              256, 0);

										GHorseWaitTime = 45;
										Front_PrintCentredPrompt(GameText.pContinue);
									} else {
										if (!GHorseSounded) {
											SFX_PlayX(42, 8192, 0);

											GHorseSounded = 1;
										}

										DAT_800a5b28->update();
										DAT_800a5b28->setScale(512);
										DAT_800a5b28->bind(1);
										Mess_DrawText(256, 152, YouAre, 0, 0, 256, 0);

										if (DAT_800a5b28->handleComplete())
											GHorseWaitTime = 45;

										DAT_800a5b28->bind(0);
										DAT_800a5b28->bind(1);
										Mess_DrawText(256, 188, GCareer.Options.HorseWord, 0, 0,
										              256, 0);
										DAT_800a5b28->bind(0);
									}

									NextHorseMode = 2;
								} else {
									YouAre[1] = 0;
									YouAre[0] = GCareer.Options.HorseWord[Letters];

									if (GHorseScale < 512) {
										GHorseScale += 32;

										if (GHorseScale > 512)
											GHorseScale = 512;

										Mess_SetScale(GHorseScale <= 384 ? GHorseScale : 384);
										Mess_DrawText(256, 152, GameText.pYouGetTheLetter, 0, 0,
										              256, 0);
										Mess_SetScale(GHorseScale <= 512 ? GHorseScale : 512);
										Mess_DrawText(256, 188, YouAre, 0, 0, 256, 0);

										GHorseWaitTime = 45;
									} else {
										if (!GHorseSounded) {
											SFX_PlayX(42, 8192, 0);

											GHorseSounded = 1;
										}

										DAT_800a5b28->update();
										DAT_800a5b28->setScale(384);
										DAT_800a5b28->bind(1);
										Mess_DrawText(256, 152, GameText.pYouGetTheLetter, 0, 0,
										              256, 0);

										if (DAT_800a5b28->handleComplete())
											GHorseWaitTime = 45;

										DAT_800a5b28->bind(0);
										DAT_800a5b28->setScale(512);
										DAT_800a5b28->bind(1);
										Mess_DrawText(256, 188, YouAre, 0, 0, 256, 0);
										DAT_800a5b28->bind(0);
									}

									if (++NextHorseRestart == NumHorseRestarts)
										NextHorseRestart = 0;

									NextHorsePlayer ^= 1;
									NextHorseMode ^= 1;
								}
							} else {
								Mess_DrawText(256, Line * 24 + 80, "SWITCH", 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
								NextHorseScore = GSkater->mPanel.mLandedTotal;
							}
						}

						Horse_DisplayWords(NextHorseLetters);
					} break;
					case 6:
					default:
						sprintf(Score1, "PLAYER ONE %d", s1);
						sprintf(Score2, "PLAYER TWO %d", s2);
						Mess_DrawText(256, 75, Score1, 0, 0, 256, 0);
						Mess_DrawText(256, 99, Score2, 0, 0, 256, 0);

						if (s2 < s1)
							Mess_DrawText(256, 123, "PLAYER ONE WINS", 0, 0, 256, 0);
						else if (s1 < s2)
							Mess_DrawText(256, 123, "PLAYER TWO WINS", 0, 0, 256, 0);
						else
							Mess_DrawText(256, 123, "DRAW", 0, 0, 256, 0);

						break;
				}
			}

			if (GGame != 7 && Front_OnTime >= 11)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 5:
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 103, GameText.pSave, 0, 0, 256, 0);

			pYesNoMenu->mY = 143;
			pYesNoMenu->Display();
			Front_PrintCentredPrompt(GameText.pToSelect);
			break;
		case 24:
			if (Front_OnTime >= 11) {
				pEHMenu->Display();
				Front_PrintCentredPrompt(GameText.pToSelect);
			}

			break;
		case 25:
			if (Front_OnTime >= 11) {
				if (GNumberOfPlayers == 2)
					pVS3Menu->EntryOff(GameText.pHighlights);
				else
					pVS3Menu->EntryOn(GameText.pHighlights);

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
			Front_PrintCentredPrompt(GameText.pToSelect);
			break;
	}
}
