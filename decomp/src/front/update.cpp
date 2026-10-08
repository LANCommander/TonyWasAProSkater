/* Front_Update__Fv (0x8001a6f8) flags=-fstructure-noalias */
/* C++ (FRONT.CPP). Fuzzy demo twin (0.66); locals and blocks from its MAIN.SYM (SoundEffect,
   SwitchOffPauseMode, Activated/GoBack/AnyButton/Start, the StartSelectPressed statics, the
   case 0 / case 20 pSkater pair, the restarts-menu head[4], the horse-word UpDown/lastAlpha/
   firstAlpha). The in-game front end: soft reset on Start+Select, then one case per paused
   status (pause menu, restarts, yes/no menus, end-of-level/heat menus, horse word, high-score
   initials, sound levels), then leaving pause mode. THPS1 drops the MPC states, so its
   EPausedStatus numbering is the demo's minus 8 from STATUS_ENDGAME on.
   Levers: -fstructure-noalias (GSkater[1] loaded above the GSkater[0] field store); Vblanks/
   Xblanks volatile; the "retry" text and the sound-level value are single ?: expressions; the
   replay choices store GVideoReplay, EndCode, then SwitchOffPauseMode (one cross-jumped tail);
   `else Front_OnTime++` tails; the sfx volumes are one chained line (demo line 3990).
   The EntryOn/EntryOff(GameText[105]) calls are each a `do { } while (0)` (macro-style):
   flow weights REG_N_REFS by loop depth, so the loop notes lift the GameText-high gcse
   pseudo's global-alloc priority (25 refs / 177 = 0.565) above the GCareer-high one
   (8 / 45 = 0.533); GameText then skips s0 (which GCareer's prefers), giving retail's s1/s2.
   */
typedef unsigned char uchar;
typedef unsigned int uint;

enum EPausedStatus {
	STATUS_NORMAL,
	STATUS_PAUSEDMENU,
	STATUS_SKIPTORESTART,
	STATUS_REALLYQUIT,
	STATUS_PADUNPLUGGED,
	STATUS_SAVEGAME,
	STATUS_QUITSAVE,
	STATUS_MEDALSAVE,
	STATUS_ENDLEVELFADESCREEN,
	STATUS_DOSAVEFADESCREEN,
	STATUS_NOSAVEFADESCREEN,
	STATUS_GOTTAPE,
	STATUS_TAPESAVE,
	STATUS_STATS,
	STATUS_ENDINGSCREEN,
	STATUS_VIDEOSCREEN,
	STATUS_VIDEOSCREEN2,
	STATUS_REALLYMAINMENU,
	STATUS_MEMCARDMENUS,
	STATUS_ENDHEAT,
	STATUS_ENDHEATREPLAY,
	STATUS_ENDHEATJUDGES,
	STATUS_ENDHEATLADDER,
	STATUS_ENDHEATWINLOSE,
	STATUS_ENDHEATMENU,
	STATUS_ENDHEATVIDEOMENU,
	STATUS_BEGINHEAT,
	STATUS_ENDGAME,
	STATUS_ENDGAMEALLGOLD,
	STATUS_ENDGAMEPLAYERMOVIE,
	STATUS_ENDGAMEBAILMOVIE,
	STATUS_ENDGAMESAVE,
	STATUS_ENDGAMECREDITS,
	STATUS_STARTHORSE,
	STATUS_CHANGEHORSEWORD,
	STATUS_LOADSUCCESS,
	STATUS_LOADFAILED,
	STATUS_HIGHSCORE,
	STATUS_SETSOUNDLEVELS
};

enum EMemCardState { SVSTATUS_CHECKINGCARDPAUSE };
enum ESaveAction { SAVE_VIDEO, SAVE_GAME };
enum EVideoMode { VIDEO_OFF, VIDEO_RECORD, VIDEO_PLAYBACK };

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle, Square, Circle, X;
	SButton LeftOne, LeftTwo, RightOne, RightTwo;
	SButton Left, Right, Up, Down;
	SButton AnalogueLeft, AnalogueRight, Start, Select;
	SButton Crouch, Jump, SmartBomb, SelectWeapon;
	uchar AnaloguePotMapping[4];
	uchar Raw[4];
	char Analogue[4];
	int Type;
	int pad150[7];
};

struct SVideoInfo {
	EVideoMode VideoMode;
	int pad4[6];
	uint NumFrames;
	int pad20[11];
	int ReplayHighlight;
	int pad50[27];
	int PlaybackFrozen;
};

struct SLevelInfo {
	int pad0[3];
	int mNoHighScores;
	int pad10[26];
};

struct SHighScore {
	char Name[4];
	uchar Skater;
	uint Score;
};

struct SCareer {
	int pad0[41];
	SHighScore HighScores[20][8]; /* 0x0a4 */
	int pad824[3];
	int SfxVol;   /* 0x830 */
	int MusicVol; /* 0x834 */
	int pad838[78];
	char PlayerName[4]; /* 0x970 */
	int pad974[18];
	int HorseLen;       /* 0x9bc */
	char HorseWord[12]; /* 0x9c0 */
};

struct SPreferences {
	int SfxVol;
	int MusicVol;
	int pad8[97];
	int HorseLen;       /* 0x18c */
	char HorseWord[12]; /* 0x190 */
};

struct CBruce {
	char pad0[0x1c];
	CBruce *mpNext; /* 0x01c */
	char pad20[0x310 - 0x20];
	int m310;
	int m314;
	char pad318[0x330 - 0x318];
	int m330;
};

struct SEntry {
	char *pString;
	short CurrentScale;
	short TargetScale;
	short HiScale;
	short LowScale;
	uchar Clearance;
	uchar Visible;
	uchar Enabled;
	uchar HiR, HiG, HiB;
	uchar LowR, LowG, LowB;
	uchar FadeUpCounter;
};

class CItem {
  public:
	virtual ~CItem();
	void *operator new(unsigned int) throw();
};

class CMenu : public CItem {
  public:
	uchar mJustification;
	uchar mRep;
	uchar mLine;
	uchar mHasCursor;
	uchar mCursorLine;
	uchar mNumLines;
	int mX;
	int mY;
	int mLineSep;
	SEntry mEntry[15];
	CMenu(int, int, uchar, int, int, int);
	void Update();
	void AddEntry(const char *);
	void EntryOn(const char *);
	void EntryOff(const char *);
	int IsEntryOn(const char *);
	int ChoiceIs(const char *);
	int FindEntry(const char *);
	void SetLine(uchar);
	void CentreY();
};

class CTape {
  public:
	char pad0[0x48];
	virtual ~CTape();
	static int DisplayContinueButton();
};

class FallingTextEffect {
  public:
	int handleComplete();
};

extern SControl Pad[2];         /* Pad */
extern SVideoInfo GVInfo1;          /* GVInfo1 */
extern SVideoInfo GVInfo2;          /* GVInfo2 */
extern SLevelInfo Levels[];        /* Levels */
extern SCareer GCareer;             /* GCareer */
extern SPreferences GPreferences;        /* GPreferences */
extern char *GameText[];             /* GameText */
extern char soundBuf[];              /* soundBuf */
extern char musicBuf[];              /* musicBuf */
extern char *CheatRestarts[];             /* CheatRestarts */
extern char *HorseRestarts[];             /* HorseRestarts */
extern CTape *GTape[];            /* GTape */
extern FallingTextEffect DAT_800a5b28[]; /* GHorseFallingText */
extern char s_de[];
extern CBruce *MechList;       /* MechList */
extern CBruce *GSkater;       /* GSkater[0] */
extern CBruce *DAT_800d17cc;       /* GSkater[1] */
extern volatile uint Vblanks; /* Vblanks */
extern volatile int Xblanks;  /* Xblanks */
extern int EndCode;           /* EndCode */
extern int GameFrozen;           /* GameFrozen */
extern int GNumberOfPlayers;           /* GNumberOfPlayers */
extern int DemoTimer;           /* DemoTimer */
extern int XFrames;
extern int GLevel; /* GLevel */
extern int GGame; /* GGame */
extern int Cheat_RestartSkip; /* Cheat_RestartSkip */
extern int CheatCheck; /* CheatCheck */
extern int GState; /* GState */
extern int GSpin; /* GSpin */
extern int GHorseClearInput2; /* GHorseClearInput2 */
extern int NumCheatRestarts; /* NumCheatRestarts */
extern int GLoadedCareerIsSaved; /* GLoadedCareerIsSaved */
extern int StartScreen;
extern int GVideoReplay; /* GVideoReplay */
extern int SizeUp;
extern int GHorseCursorPos; /* GHorseCursorPos */
extern int GHorseWaitTime; /* GHorseWaitTime */
extern int GHorseMode; /* GHorseMode */
extern int GHorseRestart; /* GHorseRestart */
extern int GHorseScore; /* GHorseScore */
extern int GHorseLetters; /* GHorseLetters[0] */
extern int GHorseLetters_4; /* GHorseLetters[1] */
extern int GHorsePlayer; /* GHorsePlayer */
extern int GHorseWordChosen; /* GHorseWordChosen */
extern int GHorseDoneStart; /* GHorseDoneStart */
extern int GClear1; /* GClear1 */
extern int GClear2; /* GClear2 */

EPausedStatus Status; /* Status */
int GSoftResetEnabled;           /* GSoftResetEnabled */
int DAT_800d14d8;           /* StartSelectPressed */
uint DAT_800d14dc;          /* VblanksWhenStartSelectFirstPressed */
int GamePaused;           /* GamePaused */
int WobbleAmplitude;           /* WobbleAmplitude */
int WobbleT;           /* WobbleT */
CMenu *pYesNoMenu;        /* pYesNoMenu */
CMenu *pESMenu;        /* pESMenu */
CMenu *pVSMenu;        /* pVSMenu */
CMenu *pVS2Menu;        /* pVS2Menu */
CMenu *pVS3Menu;        /* pVS3Menu */
CMenu *pEHMenu;        /* pEHMenu */
CMenu *pSoundMenu;        /* pSoundMenu */
CMenu *pPausedMenu;        /* pPausedMenu */
CMenu *pRestartsMenu;        /* pRestartsMenu */
int GStart1;           /* GStart1 */
int GStart2;           /* GStart2 */
int Front_OnTime;           /* Front_OnTime */
int GRollCredits;           /* GRollCredits */
int GHighScoreLineIndex;           /* GHighScoreLineIndex */
int GHighScoreInitialCursorPos;           /* GHighScoreInitialCursorPos */
int GHorseContinue;           /* GHorseContinue */
int GHorseSounded;           /* GHorseSounded */
int NextHorseMode;           /* NextHorseMode */
int NextHorsePlayer;           /* NextHorsePlayer */
int NextHorseScore;           /* NextHorseScore */
int NextHorseRestart;           /* NextHorseRestart */
int NextHorseLetters;           /* NextHorseLetters[0] */
int NextHorseLetters_4;           /* NextHorseLetters[1] */
int SFXLEVEL;           /* SFXLEVEL */
int XALEVEL;
int soundVol; /* soundVol */
int musicVol; /* musicVol */

void Front_GetButtons(int &, int &, int &, int &);
int Card_CheckStatus(int, int);
void Horse_Reset();
EPausedStatus Front_MemCardUpdate();
void Video_Stop(CBruce *);
void Video_Start(CBruce *, EVideoMode, int, int);
void Front_SwitchOnVideoScreen();
void Mess_DeleteAll();
void Pad_ClearTriggers(SControl *);
void Pad_ActuatorOff(uchar, uchar);
void SFX_Pause();
void SFX_Unpause();
int SFX_PlayX(unsigned int, short, int);
int Shell_DoCheatProcessing();
void Game_Over();
void Competition_Init();
int Utils_CompareStrings(const char *, const char *, unsigned int);
void Trig_SetRestart(char *);
void Front_SetSaveAction(ESaveAction);
void Front_InitMemCardMenus(int, EMemCardState, EPausedStatus);
bool InCompetition();
bool Competition_IsOver();
void Flash_Fade(int, int);
void Flash_Update();
int Flash_FadeFinished();
void Flash_Reset();
void Video_Cleanup();
void Video_SortAndMerge();
int GetUpDown();
void InitStatsSwisch();
void Competition_EndGameLogic(EPausedStatus &, int &);
void Competition_WinLoseLogic(EPausedStatus &, int &);
void Competition_LadderLogic();
int Competition_LadderFullyDisplayed();
int Competition_NewHeat();
void Competition_JudgeLogic();
int Competition_JudgeFullyDisplayed();
void RestartCompFrig();

extern "C" {
	int FUN_8008b900(char *, const char *, ...);
	void FUN_8008f19c(int, int, int);
	void FUN_8008f0ec(int);
	void FUN_8008f4d4(int, int, int);
	void FUN_80023528(void);
	void FUN_80076484(void);
	void FUN_80076494(void);
}

void Front_Update() {
	int SoundEffect = -1;
	int SwitchOffPauseMode = 0;
	int Activated;
	int GoBack;
	int AnyButton;
	int Start;

	if ((!Pad[0].LeftOne.Pressed || Status != STATUS_PAUSEDMENU) &&
	    Status != STATUS_MEMCARDMENUS)
		Front_GetButtons(Activated, GoBack, AnyButton, Start);

	if (Status)
		Card_CheckStatus(0, 0);

	if ((GSoftResetEnabled && Pad[0].Select.Pressed && Pad[0].Start.Pressed) ||
	    (Pad[1].Select.Pressed && Pad[1].Start.Pressed)) {
		if (!DAT_800d14d8) {
			DAT_800d14d8 = 1;
			DAT_800d14dc = Vblanks;
		}

		if (Vblanks - DAT_800d14dc > 120) {
			DAT_800d14d8 = 0;
			EndCode = 8;
			SwitchOffPauseMode = 1;
			Horse_Reset();
		}
	} else {
		DAT_800d14d8 = 0;
	}

	GSoftResetEnabled = 1;

	switch (Status) {
		case STATUS_MEMCARDMENUS:
			Status = Front_MemCardUpdate();
			break;

		case STATUS_NORMAL: {
			CBruce *pSkater = MechList;
			CBruce *pSkater2 = pSkater->mpNext;
			SVideoInfo *VInfo = &GVInfo1;

			if (VInfo->VideoMode == VIDEO_PLAYBACK && Activated) {
				VInfo->PlaybackFrozen = 0;
				GameFrozen = 0;
				Video_Stop(pSkater);

				if (pSkater2)
					Video_Stop(pSkater2);

				Front_SwitchOnVideoScreen();
				break;
			}
		}

			if ((Start || !Pad[0].Type || (GNumberOfPlayers == 2 && !Pad[1].Type)) &&
			    DemoTimer) {
				GamePaused = 1;
				GSkater->m310 = XFrames;

				if (DAT_800d17cc)
					DAT_800d17cc->m310 = XFrames;

				GSkater->m314 = XFrames;

				if (DAT_800d17cc)
					DAT_800d17cc->m314 = XFrames;

				Mess_DeleteAll();
				Pad_ClearTriggers(&Pad[0]);
				Pad_ClearTriggers(&Pad[1]);

				WobbleAmplitude = 0;
				SFX_Pause();
				SFX_PlayX(2, 4096, 0);
				Pad_ActuatorOff(0, 0);
				Pad_ActuatorOff(0, 1);

				pPausedMenu = new CMenu(256, 0, 0, 335, 256, 22);
				pPausedMenu->AddEntry(GameText[96]);
				pPausedMenu->AddEntry(
				    (Levels[GLevel].mNoHighScores && GGame == 1)
				        ? GameText[30]
						: GameText[29]);
				pPausedMenu->AddEntry(GameText[105]);

				if (!Cheat_RestartSkip)
					do {
						pPausedMenu->EntryOff(GameText[105]);
					} while (0);

				pPausedMenu->AddEntry(GameText[83]);

				if (GGame == 7)
					pPausedMenu->AddEntry(GameText[1]);
				else
					pPausedMenu->AddEntry(GameText[2]);

				pPausedMenu->CentreY();

				pPausedMenu->mY += 13;

				if (Pad[0].Type && (GNumberOfPlayers == 1 || Pad[1].Type)) {
					Status = STATUS_PAUSEDMENU;

					if (GStart1 && GStart2) {
						GClear1 = 0;
						GClear2 = 0;
					} else if (GGame != 7) {
						GClear1 = GStart2;
						GClear2 = GStart1;
					}
				} else {
					Status = STATUS_PADUNPLUGGED;
				}
			}

			break;

		case STATUS_PAUSEDMENU:
			if (!GamePaused)
				SwitchOffPauseMode = 1;

			if (!Pad[0].Type || (GNumberOfPlayers == 2 && !Pad[1].Type))
				Status = STATUS_PADUNPLUGGED;

			if (Pad[0].LeftOne.Pressed) {
				if (Shell_DoCheatProcessing()) {
					WobbleAmplitude = 32;
					WobbleT = 0;
				}

				Pad_ClearTriggers(&Pad[0]);
				Pad_ClearTriggers(&Pad[1]);
				break;
			}

			CheatCheck = 0;

			if (Cheat_RestartSkip)
				do {
					pPausedMenu->EntryOn(GameText[105]);
				} while (0);
			else
				do {
					pPausedMenu->EntryOff(GameText[105]);
				} while (0);

			pPausedMenu->Update();

			if (Activated) {
				if (pPausedMenu->ChoiceIs(GameText[2])) {
					SoundEffect = 5;
					Game_Over();

					SwitchOffPauseMode = 1;
					GState = 1;
					GSpin = 1;
				}

				if (pPausedMenu->ChoiceIs(GameText[96])) {
					SoundEffect = 5;
					SwitchOffPauseMode = 1;
				}

				if (pPausedMenu->ChoiceIs(
				        (Levels[GLevel].mNoHighScores && GGame == 1)
				            ? GameText[30]
							: GameText[29])) {
					SwitchOffPauseMode = 1;
					Horse_Reset();

					EndCode = 7;
					Competition_Init();
				}

				if (pPausedMenu->ChoiceIs(GameText[105])) {
					SoundEffect = 5;
					Status = STATUS_SKIPTORESTART;
				}

				if (pPausedMenu->ChoiceIs(GameText[98])) {
					SoundEffect = 5;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText[110]));

					Status = STATUS_REALLYQUIT;
				}

				if (pPausedMenu->ChoiceIs(GameText[83])) {
					SoundEffect = 5;
					Status = STATUS_SETSOUNDLEVELS;
					FUN_8008f19c(0, 0, -1);
					FUN_8008f0ec(480);
				}

				if (pPausedMenu->ChoiceIs(GameText[1])) {
					SoundEffect = 5;
					Front_OnTime = 0;
					GHorseClearInput2 = 0;
					Status = STATUS_ENDINGSCREEN;
				}
			}

			if (GoBack) {
				SoundEffect = 5;
				SwitchOffPauseMode = 1;
			}

			SFX_PlayX(SoundEffect, 4096, 0);
			break;

		case STATUS_SKIPTORESTART:
			if (!pRestartsMenu) {
				int i;

				pRestartsMenu = new CMenu(256, 0, 0, 335, 256, 22);
				pRestartsMenu->mLineSep = 18;

				for (i = 0; i < NumCheatRestarts; i++) {
					char head[4];

					head[0] = CheatRestarts[i][0];
					head[1] = CheatRestarts[i][1];
					head[2] = CheatRestarts[i][2];
					head[3] = 0;

					if (!Utils_CompareStrings(head, s_de, -1))
						pRestartsMenu->AddEntry(CheatRestarts[i]);
				}

				pRestartsMenu->CentreY();
			}

			pRestartsMenu->Update();

			if (Activated) {
				Trig_SetRestart(pRestartsMenu->mEntry[pRestartsMenu->mLine].pString);
				delete pRestartsMenu;
				SwitchOffPauseMode = 1;
				pRestartsMenu = 0;
				EndCode = 7;
				SFX_PlayX(5, 8192, 0);
			}

			if (GoBack) {
				delete pRestartsMenu;
				pRestartsMenu = 0;
				Status = STATUS_PAUSEDMENU;
				SFX_PlayX(2, 8192, 0);
			}

			break;

		case STATUS_REALLYQUIT:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					EndCode = 8;
					SwitchOffPauseMode = 1;
					Horse_Reset();
					SFX_PlayX(5, 8192, 0);
				} else {
					Status = STATUS_PAUSEDMENU;
					SFX_PlayX(2, 8192, 0);
				}
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				Status = STATUS_PAUSEDMENU;
			}

			break;

		case STATUS_PADUNPLUGGED:
			if (Pad[0].Type && (GNumberOfPlayers == 1 || Pad[1].Type))
				Status = STATUS_PAUSEDMENU;

			break;

		case STATUS_QUITSAVE:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					SFX_PlayX(5, 8192, 0);
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_ENDINGSCREEN);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_ENDLEVELFADESCREEN;
					SFX_PlayX(2, 8192, 0);
				}
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				Status = STATUS_ENDINGSCREEN;
			}

			break;

		case STATUS_REALLYMAINMENU:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					Status = STATUS_ENDLEVELFADESCREEN;
					SFX_PlayX(5, 8192, 0);
				} else {
					SFX_PlayX(2, 8192, 0);

					if (InCompetition() & !Competition_IsOver()) {
						Status = STATUS_ENDHEATMENU;
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText[49]));
					} else {
						Status = STATUS_ENDINGSCREEN;
					}
				}
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				if (InCompetition() & !Competition_IsOver()) {
					Status = STATUS_ENDHEATMENU;
					pEHMenu->SetLine(pEHMenu->FindEntry(GameText[49]));
				} else {
					Status = STATUS_ENDINGSCREEN;
				}
			}

			break;

		case STATUS_ENDINGSCREEN:
			if (GRollCredits) {
				Status = STATUS_ENDLEVELFADESCREEN;
			} else if (Front_OnTime > 10) {
				pESMenu->Update();

				if (Activated) {
					SFX_PlayX(5, 8192, 0);
					Flash_Fade(16, 1);

					if (pESMenu->ChoiceIs(GameText[98])) {
						if (!Levels[GLevel].mNoHighScores &&
						    pESMenu->IsEntryOn(GameText[28]) && !GLoadedCareerIsSaved) {
							Status = STATUS_QUITSAVE;
							pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText[109]));
						} else {
							Status = STATUS_REALLYMAINMENU;
						}
					} else if (pESMenu->ChoiceIs(
					               (Levels[GLevel].mNoHighScores && GGame == 1)
					                   ? GameText[30]
									   : GameText[29])) {
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Competition_Init();
						Horse_Reset();
					} else if (pESMenu->ChoiceIs(GameText[33])) {
						Status = STATUS_VIDEOSCREEN;
					} else if (pESMenu->ChoiceIs(GameText[28]) ||
					           pESMenu->ChoiceIs(GameText[89])) {
						pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText[109]));

						Status = STATUS_SAVEGAME;
					} else if (pESMenu->ChoiceIs(GameText[31])) {
						StartScreen = 1;
						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pESMenu->ChoiceIs(GameText[32])) {
						StartScreen = 2;
						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pESMenu->ChoiceIs(GameText[0])) {
						StartScreen = 6;
						Status = STATUS_ENDLEVELFADESCREEN;
					}
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_VIDEOSCREEN:
			if (Front_OnTime > 10) {
				Front_SetSaveAction(SAVE_VIDEO);
				pVSMenu->Update();

				if (Activated) {
					GVideoReplay = 0;
					SFX_PlayX(5, 8192, 0);
					Flash_Fade(32, 1);

					if (pVSMenu->ChoiceIs(GameText[27])) {
						Video_Cleanup();

						Status = STATUS_ENDINGSCREEN;
					} else if (pVSMenu->ChoiceIs(GameText[23])) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVSMenu->ChoiceIs(GameText[24])) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVSMenu->ChoiceIs(GameText[25])) {
						Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_VIDEOSCREEN);

						Status = STATUS_MEMCARDMENUS;
					}
				}

				if (GoBack) {
					SFX_PlayX(2, 8192, 0);

					Status = STATUS_ENDINGSCREEN;
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_VIDEOSCREEN2:
			if (!DemoTimer) {
				Video_Cleanup();

				Status = STATUS_ENDLEVELFADESCREEN;
			}

			if (Front_OnTime > 10) {
				Front_SetSaveAction(SAVE_VIDEO);
				pVS2Menu->Update();

				if (Activated) {
					GVideoReplay = 0;
					SFX_PlayX(5, 8192, 0);
					Flash_Fade(32, 1);

					if (pVS2Menu->ChoiceIs(GameText[27])) {
						Video_Cleanup();

						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pVS2Menu->ChoiceIs(GameText[23])) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS2Menu->ChoiceIs(GameText[24])) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					}
				}

				if (GoBack) {
					SFX_PlayX(2, 8192, 0);
					Video_Cleanup();

					Status = STATUS_ENDLEVELFADESCREEN;
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_ENDHEATVIDEOMENU:
			if (Front_OnTime > 10) {
				Front_SetSaveAction(SAVE_VIDEO);
				pVS3Menu->Update();

				if (Activated) {
					GVideoReplay = 0;
					SFX_PlayX(5, 8192, 0);
					Flash_Fade(32, 1);

					if (pVS3Menu->ChoiceIs(GameText[73])) {
						Video_Cleanup();

						Status = STATUS_ENDHEATMENU;
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText[49]));
					} else if (pVS3Menu->ChoiceIs(GameText[23])) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS3Menu->ChoiceIs(GameText[24])) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS3Menu->ChoiceIs(GameText[25])) {
						Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE,
						                       STATUS_ENDHEATVIDEOMENU);

						Status = STATUS_MEMCARDMENUS;
					}
				}

				if (GoBack) {
					SFX_PlayX(2, 8192, 0);

					Status = STATUS_ENDHEATMENU;
					pEHMenu->SetLine(pEHMenu->FindEntry(GameText[49]));
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_GOTTAPE:
			if (Front_OnTime > 10) {
				if (Activated && CTape::DisplayContinueButton()) {
					SFX_PlayX(5, 8192, 0);

					{
						int i;

						for (i = 0; i < 5; i++) {
							delete GTape[i];
							GTape[i] = 0;
						}
					}

					Status = STATUS_TAPESAVE;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText[109]));
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_CHANGEHORSEWORD: {
			int UpDown;
			char lastAlpha;
			char firstAlpha;

			GHorseClearInput2 = 0;
			UpDown = GetUpDown();
			lastAlpha = 'Z';
			firstAlpha = 'A';

			if (GHorseCursorPos >= GCareer.HorseLen - 1 && GCareer.HorseLen >= 4) {
				lastAlpha = ' ';
				firstAlpha = ' ';
			}

			if (UpDown == -1) {
				SizeUp = 16;
				SFX_PlayX(17, 8192, 0);

				if (GCareer.HorseWord[GHorseCursorPos] == ' ' ||
				    GCareer.HorseWord[GHorseCursorPos] == 0) {
					GCareer.HorseWord[GHorseCursorPos] = 'A';
				} else {
					GCareer.HorseWord[GHorseCursorPos]++;

					if ((uchar)GCareer.HorseWord[GHorseCursorPos] > 'Z')
						GCareer.HorseWord[GHorseCursorPos] = firstAlpha;
				}
			}

			if (UpDown == 1) {
				SizeUp = 16;
				SFX_PlayX(17, 8192, 0);

				if (GCareer.HorseWord[GHorseCursorPos] == ' ' ||
				    GCareer.HorseWord[GHorseCursorPos] == 0) {
					GCareer.HorseWord[GHorseCursorPos] = 'Z';
				} else {
					GCareer.HorseWord[GHorseCursorPos]--;

					if ((uchar)GCareer.HorseWord[GHorseCursorPos] < 'A')
						GCareer.HorseWord[GHorseCursorPos] = lastAlpha;
				}
			}

			GCareer.HorseLen = 10;

			{
				int i;

				for (i = 0; i < 10; i++) {
					if (GCareer.HorseWord[i] == ' ' || GCareer.HorseWord[i] == 0) {
						GCareer.HorseLen = i;
						break;
					}
				}
			}

			if (Pad[0].Left.Triggered || Pad[1].Left.Triggered) {
				SizeUp = 16;
				SFX_PlayX(5, 8192, 0);

				Pad[0].Left.Triggered = 0;
				Pad[1].Left.Triggered = 0;
				GHorseCursorPos--;

				if (GHorseCursorPos < 0)
					GHorseCursorPos = GCareer.HorseLen;

				if (GHorseCursorPos >= 10)
					GHorseCursorPos = 9;
			}

			if (Pad[0].Right.Triggered || Pad[1].Right.Triggered) {
				SizeUp = 16;
				SFX_PlayX(5, 8192, 0);

				Pad[0].Right.Triggered = 0;
				Pad[1].Right.Triggered = 0;
				GHorseCursorPos++;

				if (GHorseCursorPos > GCareer.HorseLen)
					GHorseCursorPos = 0;

				if (GHorseCursorPos >= 10)
					GHorseCursorPos = 0;
			}

			if (Activated) {
				{
					int i;

					for (i = 0; i < 10; i++) {
						if (GCareer.HorseWord[i] == ' ') {
							GCareer.HorseWord[i] = 0;
							break;
						}
					}
				}

				FUN_8008b900(GPreferences.HorseWord, GCareer.HorseWord);

				GHorseWordChosen = 1;
				GHorseDoneStart = 0;
				GPreferences.HorseLen = GCareer.HorseLen;
				SFX_PlayX(5, 8192, 0);
			}
		} break;

		case STATUS_STARTHORSE:
			GHorseClearInput2 = 1;
			GSkater->m310 = XFrames;

			if (Activated)
				SwitchOffPauseMode = 1;

			break;

		case STATUS_TAPESAVE:
			pYesNoMenu->Update();

			if (Activated) {
				SFX_PlayX(5, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_STATS);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_STATS;
				}
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				Status = STATUS_GOTTAPE;
			}

			break;

		case STATUS_HIGHSCORE:
			if (Front_OnTime > 10) {
				if (GHighScoreLineIndex != -1) {
					int UpDown = GetUpDown();

					if (UpDown == -1) {
						SizeUp = 16;
						SFX_PlayX(17, 8192, 0);

						if (GCareer.HighScores[GLevel][GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == ' ' ||
						    GCareer.HighScores[GLevel][GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == 0) {
							GCareer.HighScores[GLevel][GHighScoreLineIndex].Name[GHighScoreInitialCursorPos] =
							    'A';
						} else {
							GCareer.HighScores[GLevel][GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos]++;

							if ((uchar)GCareer.HighScores[GLevel][GHighScoreLineIndex]
							        .Name[GHighScoreInitialCursorPos] > 'Z')
								GCareer.HighScores[GLevel][GHighScoreLineIndex]
								    .Name[GHighScoreInitialCursorPos] = ' ';
						}
					}

					if (UpDown == 1) {
						SizeUp = 16;
						SFX_PlayX(17, 8192, 0);

						if (GCareer.HighScores[GLevel][GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == ' ' ||
						    GCareer.HighScores[GLevel][GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == 0) {
							GCareer.HighScores[GLevel][GHighScoreLineIndex].Name[GHighScoreInitialCursorPos] =
							    'Z';
						} else {
							GCareer.HighScores[GLevel][GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos]--;

							if ((uchar)GCareer.HighScores[GLevel][GHighScoreLineIndex]
							        .Name[GHighScoreInitialCursorPos] < 'A')
								GCareer.HighScores[GLevel][GHighScoreLineIndex]
								    .Name[GHighScoreInitialCursorPos] = ' ';
						}
					}

					if (Pad[0].Left.Triggered || Pad[1].Left.Triggered) {
						SizeUp = 16;
						SFX_PlayX(5, 8192, 0);

						Pad[0].Left.Triggered = 0;
						Pad[1].Left.Triggered = 0;
						GHighScoreInitialCursorPos--;

						if (GHighScoreInitialCursorPos < 0)
							GHighScoreInitialCursorPos = 2;
					}

					if (Pad[0].Right.Triggered || Pad[1].Right.Triggered) {
						SizeUp = 16;
						SFX_PlayX(5, 8192, 0);

						Pad[0].Right.Triggered = 0;
						Pad[1].Right.Triggered = 0;
						GHighScoreInitialCursorPos++;

						if (GHighScoreInitialCursorPos >= 3)
							GHighScoreInitialCursorPos = 0;
					}
				}

				if (Activated) {
					SFX_PlayX(18, 8192, 0);
					FUN_8008b900(GCareer.PlayerName,
					             GCareer.HighScores[GLevel][GHighScoreLineIndex].Name);

					GHighScoreLineIndex = -1;
					Status = STATUS_ENDINGSCREEN;
				}
			}

			break;

		case STATUS_STATS:
			if (Front_OnTime > 10) {
				if (GGame == 7) {
					GHorseClearInput2 = 0;

					if (--GHorseWaitTime <= 0 && NextHorseMode != 2)
						Activated = 1;

					if (NextHorseMode == 2 && GHorseContinue && !DAT_800a5b28[0].handleComplete()) {
						Front_OnTime = 0;
						Status = STATUS_ENDINGSCREEN;
					}
				}

				if (Activated) {
					SFX_PlayX(5, 8192, 0);

					GHorseSounded = 0;

					if (GGame == 7) {
						if (NextHorseMode == 2) {
							GHorseContinue = 1;
							GHorseRestart = 0;
							Trig_SetRestart(HorseRestarts[0]);
						} else {
							GHorseRestart = NextHorseRestart;
							GHorseScore = NextHorseScore;
							GHorseMode = NextHorseMode;
							GHorsePlayer = NextHorsePlayer;
							GHorseLetters = NextHorseLetters;
							GHorseLetters_4 = NextHorseLetters_4;
							SwitchOffPauseMode = 1;
							Trig_SetRestart(HorseRestarts[GHorseRestart]);

							EndCode = 7;
						}
					} else if (GHighScoreLineIndex != -1) {
						Status = STATUS_HIGHSCORE;
						InitStatsSwisch();
					} else {
						Status = STATUS_ENDINGSCREEN;
					}
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_ENDHEATMENU:
			if (Front_OnTime > 10) {
				pEHMenu->Update();

				if (Activated) {
					SFX_PlayX(5, 8192, 0);
					Flash_Fade(32, 1);

					if (pEHMenu->ChoiceIs(GameText[98])) {
						Status = STATUS_REALLYMAINMENU;
					} else if (pEHMenu->ChoiceIs(GameText[49])) {
						SwitchOffPauseMode = 1;
						EndCode = 7;
					} else if (pEHMenu->ChoiceIs(GameText[50])) {
						Status = STATUS_ENDHEATWINLOSE;
					} else if (pEHMenu->ChoiceIs(GameText[33])) {
						Status = STATUS_ENDHEATVIDEOMENU;
					}
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_ENDGAME:
			Competition_EndGameLogic(Status, Activated);
			break;

		case STATUS_ENDHEATWINLOSE:
			Competition_WinLoseLogic(Status, Activated);
			break;

		case STATUS_MEDALSAVE:
			pYesNoMenu->Update();

			if (Activated) {
				SFX_PlayX(5, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_ENDINGSCREEN);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_ENDINGSCREEN;
				}
			}

			break;

		case STATUS_ENDHEATLADDER:
			Competition_LadderLogic();

			if (Activated) {
				if (Competition_LadderFullyDisplayed()) {
					Video_Stop(MechList);

					GamePaused = 1;

					if (Competition_NewHeat()) {
						Status = STATUS_ENDHEATMENU;
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText[49]));
					} else {
						Status = STATUS_ENDHEATWINLOSE;
					}
				} else {
					FUN_80076494();
					SFX_PlayX(52, 8192, 0);
				}
			}

			break;

		case STATUS_ENDHEATREPLAY:
			Competition_JudgeLogic();

			if (Front_OnTime > 10) {
				if (Activated) {
					if (Competition_JudgeFullyDisplayed()) {
						SFX_PlayX(5, 8192, 0);

						Status = STATUS_ENDHEATLADDER;
						GVideoReplay = -1;
						RestartCompFrig();
						delete pPausedMenu;
						pPausedMenu = 0;

						if (EndCode != 8 && EndCode != 4 && EndCode != 3)
							SFX_Unpause();

						Video_SortAndMerge();

						if (GVInfo1.NumFrames > 90) {
							CBruce *pSkater = MechList;
							CBruce *pSkater2 = pSkater->mpNext;

							GamePaused = 0;
							GVideoReplay = 0;
							GVInfo1.ReplayHighlight = 0;
							GVInfo2.ReplayHighlight = 0;
							Video_Start(pSkater, VIDEO_PLAYBACK, 1, 0x7fffffff);

							if (pSkater2)
								Video_Start(pSkater2, VIDEO_PLAYBACK, 1, 0x7fffffff);

							GState = 0;
							Flash_Reset();

							MechList->m330 = Xblanks + 216030;

							if (MechList->mpNext)
								MechList->mpNext->m330 = Xblanks + 216030;
						}
					} else {
						FUN_80076484();
						SFX_PlayX(52, 8192, 0);
					}
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_ENDHEAT:
			Status = STATUS_ENDHEATREPLAY;
			Front_OnTime++;
			break;

		case STATUS_SAVEGAME:
			pYesNoMenu->Update();

			if (Activated) {
				SFX_PlayX(5, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText[109])) {
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_ENDINGSCREEN);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_ENDINGSCREEN;
				}
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				Status = STATUS_ENDINGSCREEN;
			}

			break;

		case STATUS_ENDLEVELFADESCREEN:
			Flash_Update();

			if (Flash_FadeFinished()) {
				EndCode = 8;
				SwitchOffPauseMode = 1;
				Horse_Reset();
			}

			break;

		case STATUS_DOSAVEFADESCREEN:
		case STATUS_NOSAVEFADESCREEN:
			Flash_Update();

			if (Flash_FadeFinished()) {
				if (Status == STATUS_DOSAVEFADESCREEN)
					EndCode = 4;
				else
					EndCode = 3;

				SwitchOffPauseMode = 1;
			}

			break;

		case STATUS_SETSOUNDLEVELS:
			pSoundMenu->Update();

			XALEVEL =
			    (pSoundMenu->ChoiceIs(musicBuf) || pSoundMenu->ChoiceIs(GameText[86]))
			        ? GPreferences.MusicVol * 0x555
			        : 0;

			if (Pad[0].Left.Triggered || Pad[1].Left.Triggered) {
				Pad[0].Left.Triggered = 0;
				Pad[1].Left.Triggered = 0;

				if (pSoundMenu->ChoiceIs(soundBuf)) {
					if (soundVol > 0)
						soundVol--;

					GPreferences.SfxVol = GCareer.SfxVol = soundVol * 6 / 5;
					FUN_8008b900(soundBuf, GameText[84], soundVol);

					SFXLEVEL = GCareer.SfxVol * 0x555;
					SFX_PlayX(5, 8192, 0);
				} else if (pSoundMenu->ChoiceIs(musicBuf)) {
					if (musicVol > 0)
						musicVol--;

					GPreferences.MusicVol = GCareer.MusicVol = musicVol * 6 / 5;
					FUN_8008b900(musicBuf, GameText[85], musicVol);
				}
			}

			if (Pad[0].Right.Triggered || Pad[1].Right.Triggered) {
				Pad[0].Right.Triggered = 0;
				Pad[1].Right.Triggered = 0;

				if (pSoundMenu->ChoiceIs(soundBuf)) {
					if (soundVol < 10)
						soundVol++;

					GPreferences.SfxVol = GCareer.SfxVol = soundVol * 6 / 5;
					FUN_8008b900(soundBuf, GameText[84], soundVol);

					SFXLEVEL = GCareer.SfxVol * 0x555;
					SFX_PlayX(5, 8192, 0);
				} else if (pSoundMenu->ChoiceIs(musicBuf)) {
					if (musicVol < 10)
						musicVol++;

					GPreferences.MusicVol = GCareer.MusicVol = musicVol * 6 / 5;
					FUN_8008b900(musicBuf, GameText[85], musicVol);
				}
			}

			if (Activated) {
				if (pSoundMenu->ChoiceIs(GameText[86]))
					FUN_80023528();
				else if (pSoundMenu->ChoiceIs(GameText[87]))
					GoBack = 1;
			}

			if (GoBack) {
				SFX_PlayX(2, 8192, 0);

				Status = STATUS_PAUSEDMENU;
				XALEVEL = GCareer.MusicVol * 0x555;
				FUN_8008f19c(1, 0, -1);
				FUN_8008f4d4(9, 0, 0);
			}

			break;
	}

	if (SwitchOffPauseMode) {
		GamePaused = 0;
		GClear1 = 0;
		GClear2 = 0;
		Status = STATUS_NORMAL;
		delete pPausedMenu;
		pPausedMenu = 0;

		if (EndCode != 8 && EndCode != 4 && EndCode != 3)
			SFX_Unpause();
	}
}
