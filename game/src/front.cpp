/* FRONT.CPP: the in-game front end: CMenu (the scaling text menus), the pause, end-of-run,
   replay, end-of-heat, high-score, H.O.R.S.E. and sound-level screens (Front_Display /
   Front_Update), the front end's buttons, and loading a level's saved game state (declarations
   in game/front.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "pc_screen.h"
#include "game/front.h"
#include "game/career.h"
#include "game/m3d.h"
#include "game/mess.h"
#include "game/pad.h"
#include "game/psyq.h"
#include "game/bruce.h"
#include "game/tapes.h"
#include "game/panel.h"
#include "game/shell.h"
#include "game/padbuttons.h"
#include "game/classsizes.h"
#include "game/video.h"
#include "game/competition.h"
#include "game/gamefmv.h"
#include "pc_stream.h"
#include "game/card.h"
#include "game/shellutils.h"
#include "game/bit.h"
#include "game/camera.h"
#include "game/fileio.h"
#include "game/flash.h"
#include "game/font.h"
#include "game/frontend.h"
#include "game/globals.h"
#include "game/hires.h"
#include "game/main.h"
#include "game/physics.h"
#include "game/powerup.h"
#include "game/rfront.h"
#include "game/sfx.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/utils.h"

struct STrig {
	short s, c;
};

extern int DAT_800d13ec;
extern int GHorseSounded;
extern int GHorseMode;
extern int GHorseScore;
extern int GHorseCursorPos;
extern int StatsRight[];
extern int StatsLeft[];
extern char DAT_8009e548[];
/* A C name for a declaration (as game/data's PSX_NAME): `__asm`, not `__asm__`, which portsrc's
   inline-asm translation would take for MIPS code. */
#define FRONT_STR_(x) #x
#define FRONT_STR(x) FRONT_STR_(x)
#define FRONT_PSX_NAME(name) __asm(FRONT_STR(__USER_LABEL_PREFIX__) name)
/* Pad again, without its count, for CMenu::Update and Front_GetButtons: their code was built
   against `SControl Pad[]`, and gcc merges the two pads' tests when it knows both are there */
extern SControl Pad_[] FRONT_PSX_NAME("Pad");
extern int UpDownAutoRepeatTimer;
extern RECT DAT_800d1444[];
extern SSaveGame DAT_8009e31c;
extern char soundBuf[];
extern char musicBuf[];
extern CMenu *pESMenu;
extern CMenu *pVSMenu;
extern CMenu *pVS2Menu;
extern CMenu *pVS3Menu;
extern CMenu *pEHMenu;
extern CMenu *pSoundMenu;
extern CMenu *pPausedMenu;
extern CMenu *pRestartsMenu;
extern int soundVol, musicVol;
extern void (*DAT_800d1408)(void);
extern int Cheat_GameInfo;
extern int GHorseWaitTime;
extern FallingTextEffect DAT_800a5b28[]; /* GHorseFallingText */
extern int WobbleAmplitude;
extern int WobbleT;
extern int Front_OnTime;
extern int GHighScoreLineIndex;
extern int GHighScoreInitialCursorPos;
extern int GHorseContinue;
extern int GHorseScale;
extern int NextHorseMode;
extern int NextHorsePlayer;
extern int NextHorseScore;
extern int NextHorseRestart;
extern int NextHorseLetters[2];
extern int GStart1, GStart2;
extern int DAT_800d1e48;
extern int DAT_800d1524;
extern int DAT_800d1528;
extern int DAT_800d152c;
extern int DAT_800d14d8; /* StartSelectPressed */
extern uint DAT_800d14dc;
extern char *TrigFileName;
extern Sprite *pSpecialSprite;
extern Sprite *DAT_800d1858; /* the "paused" sprite */

void __dl__6CClassPv(void *p);
void Front_PrintGoBack();
void Front_PrintSelect();
void Graffiti_Stats(CBruce *, int, int, int, int, int, int, int, int, int);
void InitStatsSwisch();
/* the two as Front_Update was built against them (the demo's): returning bool */
typedef bool (*FrontBoolFn)();
#define InCompetition_Bool() (((FrontBoolFn)InCompetition)())
#define Competition_IsOver_Bool() (((FrontBoolFn)Competition_IsOver)())
void Front_SavePreferences(SPreferences &);
uint Front_CalculateGameChecksum(SSaveGame &);
SLevel *Front_FindLevel(char *);
extern "C" void FUN_80071fa4(void);
/* FallingTextEffect's destructor (with its delete flag) and constructor, called by name for the
   static GHorseFallingText */
extern "C" void FUN_8001dfec(void *, int);
extern "C" void __17FallingTextEffect(void *);

/* Front_PrintDemo__Fv (0x800160bc) */
void Front_PrintDemo(void) {
	if (--DAT_800d13ec == 0)
		DAT_800d13ec = 48;

	if (DAT_800d13ec >= 13) {
		if (GNumberOfPlayers == 2) {
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(128, 35, "demo mode", 1, 0, 256, 0);
			Mess_DrawText(384, 35, "demo mode", 1, 0, 256, 0);
		} else {
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 35, "demo mode", 1, 0, 256, 0);
		}
	}
}

/* Front_DisplayScreen__Fv (0x800161d8) */
void Front_DisplayScreen(void) {
	int i;

	HiRes_BlankScreen();
	Mess_LoadFont("ingame1.fnt", -1, -1, -1);
	PC_ScreenEnable(1);

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();

		GameFade = 0;
		SetClippingBox(0, 0, 640, 480);
		Mess_SetTextJustify(0);
		Mess_SetScale(384);
		Mess_SetRGB(150, 100, 25, 0);
		Mess_DrawText(320, 240, "loading...", 1, 0, 256, 0);
		HiRes_DrawOTag();
	} while (i < 2);

	Mess_UnloadFont();
}

/* Horse_Reset__Fv (0x800162bc) */
void Horse_Reset(void) {
	GHorseSounded = 0;
	GHorsePlayer = 0;
	GHorseMode = 0;
	GHorseScore = 0;
	GHorseDoneStart = 0;
	GHorseRestart = 0;
	GHorseLetters[0] = 0;
	GHorseLetters[1] = 0;
	GHorseCursorPos = 0;
	GHorseWordChosen = 0;
	GHorseClearInput2 = 1;
}

/* Horse_DisplayWords__FPi (0x8001631c) */
void Horse_DisplayWords(int *p) {
	char buf[32];
	int i;

	Mess_SetScale(256);
	Mess_SetTextJustify(0);

	for (i = 0; i < 2; i++) {
		char *s = GCareer.Options.HorseWord;
		char *o = buf;
		int n = p[i];

		if (*s) {
			do {
				if (n) {
					n--;
					*o++ = *s++;
				} else {
					*o++ = '-';
					s++;
				}
			} while (*s);
		}

		*o = 0;

		if (i == 0) {
			Mess_SetRGB(80, 80, 128, 0);
			Mess_DrawText(86, 32, "PLAYER 1", 0, 0, 256, 0);
		} else {
			Mess_SetRGB(128, 80, 80, 0);
			Mess_DrawText(426, 32, "PLAYER 2", 0, 0, 256, 0);
		}

		Mess_DrawText(426 - (1 - i) * 340, 48, buf, 0, 0, 256, 0);
	}
}

/* InitStatsSwisch__Fv (0x8001648c) */
void InitStatsSwisch(void) {
	int i;

	for (i = 0; i < 8; i++) {
		int v = (4 - i) * 30;

		StatsLeft[i] = -100 - v;
		StatsRight[i] = v + 612;
	}
}

/* InCompetition__Fv (0x800164e4) */
int InCompetition() {
	if (Levels[GLevel].isCompetition != 0 && GGame == 1)
		return 1;

	return 0;
}

/* __5CMenuiiUciii (0x80016534) */
/* Sets the menu's position, justification and line separation, gives every line
   the default highlight (orange) and normal (grey) colours and scales, and selects line 0. */
CMenu::CMenu(int x, int y, uchar justification, int hiScale, int lowScale, int lineSep) {
	mX = x;
	mY = y;
	mJustification = justification;
	mLineSep = lineSep;

	for (int i = 0; i < 15; i++) {
		mEntry[i].HiR = 182;
		mEntry[i].HiG = 132;
		mEntry[i].HiB = 36;
		mEntry[i].LowR = 122;
		mEntry[i].LowG = 122;
		mEntry[i].LowB = 122;
		mEntry[i].FadeUpCounter = 128;
		mEntry[i].HiScale = hiScale;
		mEntry[i].LowScale = lowScale;
	}

	SetLine(0);
}

/* FUN_8001664c (0x8001664c) */
void FUN_8001664c(char *p, int f) {
	*(char **)(p + 0) = DAT_8009e548;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* CentreY__5CMenu (0x80016680) */
/* Centres the visible lines (line separation plus each line's clearance) on the
   240-line screen. */
void CMenu::CentreY() {
	int count = 0;
	int height = 0;

	for (int i = 0; i < mNumLines; i++) {
		if (mEntry[i].Visible) {
			count++;
			height += mEntry[i].Clearance;
		}
	}

	mY = (240 - ((count - 1) * mLineSep + height)) / 2;
}

/* SetLine__5CMenuUc (0x80016708) */
/* Demo twin (same lines). Selects menu line Line: it scales up, the rest down. */
void CMenu::SetLine(uchar Line) {
	mLine = Line;

	for (int i = 0; i < 15; i++) {
		mEntry[i].TargetScale = (i == mLine) ? mEntry[i].HiScale : mEntry[i].LowScale;
		mEntry[i].CurrentScale = mEntry[i].TargetScale;
	}
}

/* FindEntry__5CMenuPCc (0x80016770) */
/* Index of the line whose text is pString (0 if none). */
int CMenu::FindEntry(const char *pString) {
	for (int i = 0; i < mNumLines; i++)
		if (Utils_CompareStrings(pString, mEntry[i].pString, 0xffffffff))
			return i;

	return 0;
}

/* AddEntry__5CMenuPCc (0x800167f0) */
/* Appends a visible, enabled line with no extra clearance. */
void CMenu::AddEntry(const char *pString) {
	mEntry[mNumLines].pString = (char *)pString;
	mEntry[mNumLines].Visible = 1;
	mEntry[mNumLines].Enabled = 1;
	mEntry[mNumLines].Clearance = 0;
	mNumLines++;
}

/* EntryOn__5CMenuPCc (0x80016878) */
/* Shows the line whose text is pString. */
void CMenu::EntryOn(const char *pString) {
	mEntry[FindEntry(pString)].Visible = 1;
}

/* EntryOff__5CMenuPCc (0x800168b4) */
/* Hides the line whose text is pString. */
void CMenu::EntryOff(const char *pString) {
	mEntry[FindEntry(pString)].Visible = 0;
}

/* IsEntryOn__5CMenuPCc (0x800168ec) */
/* Whether the line whose text is pString is shown. */
int CMenu::IsEntryOn(const char *pString) {
	return mEntry[FindEntry(pString)].Visible == 1;
}

/* ChoiceIs__5CMenuPCc (0x8001692c) */
/* Whether the current line's text is pString. */
int CMenu::ChoiceIs(const char *pString) {
	return Utils_CompareStrings(pString, mEntry[mLine].pString, 0xffffffff);
}

/* Display__5CMenu (0x8001696c) */
/* Demo twin (0.97); members, locals and blocks from its MAIN.SYM. Draws each
   visible menu line at its current scale: the selected line in its highlight colour, the others
   fading up from half brightness; then, for the cursor line of a left-justified menu, a pair of
   red arrowheads either side of the text. Needs -fstructure-noalias: retail loads pDoubleBuffer
   (the OT) above the right arrow's halfword stores. */
void CMenu::Display() {
	int y;

	Mess_SetTextJustify(mJustification);

	y = mY;

	{
		int i;

		for (i = 0; i < mNumLines; i++) {
			y += mEntry[i].Clearance;

			if (mEntry[i].Visible) {
				if (mEntry[i].CurrentScale > 0) {
					Mess_SetScale(mEntry[i].CurrentScale);

					if (i == mLine) {
						Mess_SetRGB(mEntry[i].HiR, mEntry[i].HiG, mEntry[i].HiB, 0);

						mEntry[i].FadeUpCounter = 0;
					} else {
						if (mEntry[i].Enabled)
							mEntry[i].FadeUpCounter = 128;
						else
							mEntry[i].FadeUpCounter = 64;

						Mess_SetRGB(mEntry[i].LowR * mEntry[i].FadeUpCounter >> 7,
						            mEntry[i].LowG * mEntry[i].FadeUpCounter >> 7,
						            mEntry[i].LowB * mEntry[i].FadeUpCounter >> 7, 0);

						mEntry[i].FadeUpCounter += 60;

						if (mEntry[i].FadeUpCounter > 128)
							mEntry[i].FadeUpCounter = 128;
					}

					if (mEntry[i].LowScale < mEntry[i].CurrentScale)
						Mess_SetSort(0);
					else
						Mess_SetSort(1);

					{
						int length = Mess_DrawText(mX, y, mEntry[i].pString, 0, 0, 0, 0);

						if (mHasCursor && i == mCursorLine && !mJustification) {
							int off = length * mEntry[i].CurrentScale / 512 +
							          mEntry[i].CurrentScale * 14 / 256;
							POLY_F3 *leftpoly = (POLY_F3 *)pPoly;
							POLY_F3 *rightpoly = leftpoly + 1;
							uchar *newppoly = (uchar *)(rightpoly + 1);

							if (newppoly <= PolyBufferEnd) {
								pPoly = newppoly;
								setPolyF3(leftpoly);
								setPolyF3(rightpoly);
								setRGB0(leftpoly, 255, 0, 0);

								leftpoly->x0 = mX - off;
								leftpoly->y0 = y;
								leftpoly->x1 = leftpoly->x0 - 20;
								leftpoly->y1 = y - 6;
								leftpoly->x2 = leftpoly->x0 - 20;
								leftpoly->y2 = y + 6;
								setRGB0(rightpoly, 255, 0, 0);

								rightpoly->x0 = mX + off;
								rightpoly->y0 = y;
								rightpoly->x1 = rightpoly->x0 + 20;
								rightpoly->y1 = y - 6;
								rightpoly->x2 = rightpoly->x0 + 20;
								rightpoly->y2 = y + 6;

								{
									P_TAG *ot = &pDoubleBuffer->OT2D;

									addPrim(ot, leftpoly);
									addPrim(ot, rightpoly);
								}
							}
						}
					}
				}

				y += mLineSep;
			}
		}
	}
}

/* Update__5CMenu (0x80016dc0) */
/* Types from the demo's MAIN.SYM. */
void CMenu::Update() {
	while (!mEntry[mLine].Visible) {
		mEntry[mLine].TargetScale = mEntry[mLine].LowScale;
		mEntry[mLine].CurrentScale = mEntry[mLine].LowScale;
		mLine++;

		if (mLine >= mNumLines)
			mLine = 0;

		mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
		mEntry[mLine].CurrentScale = mEntry[mLine].LowScale;
	}

	if (Pad_[0].MenuUp.Pressed || Pad_[0].MenuDown.Pressed || Pad_[1].MenuUp.Pressed ||
	    Pad_[1].MenuDown.Pressed) {
		if (UpDownAutoRepeatTimer == 0 ||
		    (UpDownAutoRepeatTimer >= 16 && (UpDownAutoRepeatTimer & 3) == 0)) {
			if (Pad_[0].MenuUp.Pressed || Pad_[1].MenuUp.Pressed) {
				mEntry[mLine].TargetScale = mEntry[mLine].LowScale;

				do {
					if (mLine == 0)
						mLine = mNumLines - 1;
					else
						mLine--;
				} while (!mEntry[mLine].Visible || !mEntry[mLine].Enabled);

				mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
				SFX_PlayX(SND_OLLIE, 8192, 0);
			}

			if (Pad_[0].MenuDown.Pressed || Pad_[1].MenuDown.Pressed) {
				mEntry[mLine].TargetScale = mEntry[mLine].LowScale;

				do {
					mLine++;

					if (mLine >= mNumLines)
						mLine = 0;
				} while (!mEntry[mLine].Visible || !mEntry[mLine].Enabled);

				mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
				SFX_PlayX(SND_OLLIE, 8192, 0);
			}
		}

		UpDownAutoRepeatTimer++;
	} else {
		UpDownAutoRepeatTimer = 0;
	}

	for (int i = 0; i < mNumLines; i++) {
		if (mEntry[i].CurrentScale < mEntry[i].TargetScale) {
			mEntry[i].CurrentScale += 40;

			if (mEntry[i].CurrentScale > mEntry[i].TargetScale)
				mEntry[i].CurrentScale = mEntry[i].TargetScale;
		}

		if (mEntry[i].CurrentScale > mEntry[i].TargetScale) {
			mEntry[i].CurrentScale -= 40;

			if (mEntry[i].CurrentScale < mEntry[i].TargetScale)
				mEntry[i].CurrentScale = mEntry[i].TargetScale;
		}
	}
}

/* Front_ClearScreen__Fv (0x80017148) */
void Front_ClearScreen(void) {
	RECT r;

	GameFade = 0;
	r = DAT_800d1444[0];
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);
}

/* Front_SavePreferences__FR12SPreferences (0x800171e0) */
void Front_SavePreferences(SPreferences &prefs) {}

/* Front_SetPreferences__FRC12SPreferences (0x800171e8) */
void Front_SetPreferences(const SPreferences &prefs) {}

/* Front_Init__Fv (0x800171f0) */
void Front_Init(void) {
	int q, q2;
	CMenu *a1, *a2, *a3, *a4, *a5, *a6, *a7;

	DAT_800d1408 = FUN_80071fa4;
	StartupGameState = DAT_8009e31c;
	LastGameState = DAT_8009e31c;
	a1 = new CMenu(256, 0, 0, 335, 256, 22);

	pYesNoMenu = a1;
	a1->AddEntry(GameText.pYes);
	pYesNoMenu->AddEntry(GameText.pNo);

	a2 = new CMenu(256, 0, 0, 335, 256, 22);

	pESMenu = a2;
	a2->AddEntry(GameText.pUpdateSaveFile);
	pESMenu->AddEntry((Levels[GLevel].isCompetition && GGame == 1) ? GameText.pRestart : GameText.pRetry);
	pESMenu->AddEntry(GameText.pLevelHints);
	pESMenu->AddEntry(GameText.pSelectLevel);
	pESMenu->AddEntry(GameText.pEquipmentSelect);
	pESMenu->AddEntry(GameText.pWatchReplay);
	pESMenu->AddEntry(GameText.pSaveCareer);
	pESMenu->AddEntry(GameText.pQuit);

	if (GGame != 1) {
		pESMenu->EntryOff(GameText.pSaveCareer);

		if (GGame != 1)
			goto off0;
	}

	if (Levels[GLevel].isCompetition) {
	off0:
		pESMenu->EntryOff(GameText.pLevelHints);
	}

	if (GGame != 3)
		pESMenu->EntryOff(GameText.pUpdateSaveFile);

	if (GGame == 2 || GGame == 7)
		pESMenu->EntryOff(GameText.pWatchReplay);

	pESMenu->CentreY();
	pESMenu->SetLine(
	    pESMenu->FindEntry((Levels[GLevel].isCompetition && GGame == 1) ? GameText.pRestart
	                                                                     : GameText.pRetry) &
	    255);

	a3 = new CMenu(256, 0, 0, 335, 256, 22);

	pVSMenu = a3;
	a3->AddEntry(GameText.pFullReplay);
	pVSMenu->AddEntry(GameText.pHighlights);
	pVSMenu->AddEntry(GameText.pSaveVideo);
	pVSMenu->AddEntry(GameText.pExit);
	pVSMenu->CentreY();

	a4 = new CMenu(256, 0, 0, 335, 256, 22);

	pVS2Menu = a4;
	a4->AddEntry(GameText.pFullReplay);
	pVS2Menu->AddEntry(GameText.pHighlights);
	pVS2Menu->AddEntry(GameText.pExit);
	pVS2Menu->CentreY();

	a5 = new CMenu(256, 0, 0, 335, 256, 22);

	pVS3Menu = a5;
	a5->AddEntry(GameText.pFullReplay);
	pVS3Menu->AddEntry(GameText.pHighlights);
	pVS3Menu->AddEntry(GameText.pSaveVideo);
	pVS3Menu->AddEntry(GameText.pReturnToCompetition);
	pVS3Menu->CentreY();

	a6 = new CMenu(256, 0, 0, 335, 256, 22);

	pEHMenu = a6;
	a6->AddEntry(GameText.pNextHeat);
	pEHMenu->AddEntry(GameText.pWatchReplay);
	pEHMenu->AddEntry(GameText.pEndComp);
	pEHMenu->AddEntry(GameText.pQuit);
	pEHMenu->CentreY();

	a7 = new CMenu(256, 0, 0, 335, 256, 22);

	pSoundMenu = a7;
	q = (GCareer.Options.SFXVolume + 1) * 10 / 12;
	soundVol = q;
	sprintf(soundBuf, GameText.pSFXLevel, q);
	pSoundMenu->AddEntry(soundBuf);

	q2 = (GCareer.Options.MusicVolume + 1) * 10 / 12;
	musicVol = q2;
	sprintf(musicBuf, GameText.pMusicLevel, q2);
	pSoundMenu->AddEntry(musicBuf);
	pSoundMenu->AddEntry(GameText.pSkipTrack);
	pSoundMenu->AddEntry(GameText.pBackToMainMenu);
	pSoundMenu->CentreY();
}

/* PrintPaused__Fv (0x80017898) */
void PrintPaused(void) {
	OT_entry = 1;
	DAT_800d1858->draw(128, 20, 1, -1, -1, -1);
}

/* Front_PrintGoBack__Fv (0x800178e4) */
void Front_PrintGoBack(void) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(92, 214, GameText.pToGoBack, 0, 0, 256, 0);
	Mess_SetTextJustify(0);

	p = Panel_DrawTexturedPoly(ButtonsAnim + 1);

	if (p)
		Panel_SetStretchedScreenCoords(64, 211, p, ButtonsAnim + 1);
}

/* Front_PrintSelect__Fv (0x80017978) */
void Front_PrintSelect(void) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(348, 214, GameText.pToSelect, 0, 0, 256, 0);
	Mess_SetTextJustify(0);

	p = Panel_DrawTexturedPoly(ButtonsAnim);

	if (p)
		Panel_SetStretchedScreenCoords(320, 211, p, ButtonsAnim);
}

/* Front_PrintGoBackSelect__Fv (0x80017a08) */
void Front_PrintGoBackSelect(void) {
	Front_PrintGoBack();
	Front_PrintSelect();
}

/* Front_PrintCentredPrompt__FPCc (0x80017a30) */
void Front_PrintCentredPrompt(const char *s) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(227, 214, s, 0, 0, 256, 0);

	p = Panel_DrawTexturedPoly(ButtonsAnim);

	if (p)
		Panel_SetStretchedScreenCoords(192, 211, p, ButtonsAnim);
}

/* Graffiti_Stats__FP6CBruceiiiiiiiii (0x80017abc) */
/* C++. The graffiti game's scoreboard of one player: labels, then the skater's tags and steals
   and the high score, wins, losses and streak the caller passes. */
void Graffiti_Stats(CBruce *b, int x, int y, int r, int g, int bl, int v1, int v2, int v3, int v4) {
	char buf[16];

	Mess_SetRGB(r, g, bl, 0);
	Mess_SetTextJustify(1);
	Mess_DrawText(x + 20, y, "TAGS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 17, "STEALS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 51, "HIGH", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 68, "WINS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 85, "LOSSES", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 102, "STREAK", 0, 0, 256, 0);
	Mess_SetTextJustify(2);
	sprintf(buf, "%d", b->mGraffitiCount);
	Mess_DrawText(x + 136, y, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", b->mGraffitiSteals);
	Mess_DrawText(x + 136, y + 17, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v4);
	Mess_DrawText(x + 136, y + 51, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v1);
	Mess_DrawText(x + 136, y + 68, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v2);
	Mess_DrawText(x + 136, y + 85, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v3);
	Mess_DrawText(x + 136, y + 102, buf, 0, 0, 256, 0);
}

/* Front_Display__Fv (0x80017d70) */
/* Fuzzy demo twin (0.55); locals and statics from its MAIN.SYM. Draws whatever
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
void Front_Display() {
	CBruce *pMan;
	static char Score1[20];
	static char Score2[20];
	static char Tapes[12]; /* "%d of %d": 5 bytes was too small for "5 of 5" */
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
			int NumTapes = GSkater[0]->mNumTapes;

			if (!GTape[0]) {
				SFX_PlayX(SND_COLLECT70, 8192, 0);

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
						    Career_Progress(GSkater[0]->mSkaterIndex)->NumTapes -
						    (NumTapes - i);
						GTape[i]->mSessionTapeNum = i;
					}
				}
			}

			{
				int i;

				for (i = 0; i < 5; i++) {
					if (GTape[i])
						GTape[i]->AI(Career_Progress(GSkater[0]->mSkaterIndex)->NumTapes -
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
				highScoreBuf[0] = Career_HighScores(GLevel)[i].Name[0];
				Mess_SetTextJustify(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 0)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText(StatsLeft[i] - 80, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = Career_HighScores(GLevel)[i].Name[1];
				Mess_SetTextJustify(0);

				if (i == GHighScoreLineIndex && GHighScoreInitialCursorPos == 1)
					Mess_SetScale(SizeUp * 6 + 256);
				else
					Mess_SetScale(256);

				Mess_DrawText(StatsLeft[i] - 60, (i + 1) * 19 + 40, highScoreBuf, 0, 0, 256, 0);
				Mess_SetScale(256);

				highScoreBuf[0] = Career_HighScores(GLevel)[i].Name[2];
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

				sprintf(highScoreBuf, "%s", Stats[Career_HighScores(GLevel)[i].Skater].Name);

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
				sprintf(highScoreBuf, "%d", Career_HighScores(GLevel)[i].Score);
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
				sprintf(Score2, "%d", Career_HighScores(GLevel)[0].Score);
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

				s1 = GSkater[0]->mPanel.mTotalScore;
				s2 = 0;

				if (DAT_800d17cc)
					s2 = DAT_800d17cc->mPanel.mTotalScore;

				switch (GGame) {
					case 8:
						if (DAT_800d17cc->mGraffitiCount == GSkater[0]->mGraffitiCount)
							Mess_DrawText(256, 64, "TIE", 0, 0, 256, 0);
						else if (GSkater[0]->mGraffitiCount < DAT_800d17cc->mGraffitiCount)
							Mess_DrawText(256, 64, "PLAYER TWO WINS", 0, 0, 256, 0);
						else
							Mess_DrawText(256, 64, "PLAYER ONE WINS", 0, 0, 256, 0);

						Mess_SetScale(224);
						Graffiti_Stats(GSkater[0], 30, 86, 32, 32, 128, GWins1, GLosses1, GStreak1,
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
							GHorseScore = GSkater[0]->mPanel.mLandedTotal;

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
							sprintf(Score, "YOU SCORED %d", GSkater[0]->mPanel.mLandedTotal);
							Mess_DrawText(256, Line++ * 24 + 80, Score, 0, 0, 256, 0);

							if (GSkater[0]->mPanel.mLandedTotal == (uint)GHorseScore) {
								Mess_DrawText(256, Line * 24 + 80, "PUSH", 0, 0, 256, 0);

								NextHorsePlayer ^= 1;
							} else if (GSkater[0]->mPanel.mLandedTotal < (uint)GHorseScore) {
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
											SFX_PlayX(SND_GLASS1, 8192, 0);

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
											SFX_PlayX(SND_GLASS1, 8192, 0);

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
								NextHorseScore = GSkater[0]->mPanel.mLandedTotal;
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

/* Front_GetButtons__FRiN30 (0x80019e94) */
/* The front end's buttons this frame, each consumed: a = activate (menu accept, or Start on the
   PS1), b = go back (cleared when a is set), c = any button, d = Start/pause (and GStart1/2 say
   which pad pressed it). */
void Front_GetButtons(int &a, int &b, int &c, int &d) {
	int v, w, e;

	if (Cheat_ScreenShots == 0 || GameFrozen == 0) {
		v = 0;

		if (Pad_[0].Pause.Triggered || Pad_[1].Pause.Triggered)
			v = 1;

		d = v;
		e = Pad_[0].Pause.Triggered;
		GStart1 = e;

		if (!e)
			GStart2 = Pad_[1].Pause.Triggered;
		else
			GStart2 = 0;

		Pad_[0].Pause.Triggered = 0;
		Pad_[1].Pause.Triggered = 0;
	} else {
		d = 0;
	}

	v = 0;

	/* Pause is bound on its own on PC (often to the same key as Back), so it doesn't activate */
	if (Pad_[0].MenuAccept.Triggered || Pad_[1].MenuAccept.Triggered)
		v = 1;

	a = v;
	Pad_[0].MenuAccept.Triggered = 0;
	Pad_[1].MenuAccept.Triggered = 0;
	w = 0;

	if (Pad_[0].MenuBack.Triggered || Pad_[1].MenuBack.Triggered)
		w = 1;

	b = w;
	Pad_[0].MenuBack.Triggered = 0;
	Pad_[1].MenuBack.Triggered = 0;
	c = b | a;

	if (Pad_[0].Flip.Triggered) {
		Pad_[0].Flip.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].Grab.Triggered) {
		Pad_[0].Grab.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].Camera.Triggered) {
		Pad_[0].Camera.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].SpinLeft.Triggered) {
		Pad_[0].SpinLeft.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].TapSpinLeft.Triggered) {
		Pad_[0].TapSpinLeft.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].SpinRight.Triggered) {
		Pad_[0].SpinRight.Triggered = 0;
		c = 1;
	}

	if (Pad_[0].TapSpinRight.Triggered) {
		Pad_[0].TapSpinRight.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].Flip.Triggered) {
		Pad_[1].Flip.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].Grab.Triggered) {
		Pad_[1].Grab.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].Camera.Triggered) {
		Pad_[1].Camera.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].SpinLeft.Triggered) {
		Pad_[1].SpinLeft.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].TapSpinLeft.Triggered) {
		Pad_[1].TapSpinLeft.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].SpinRight.Triggered) {
		Pad_[1].SpinRight.Triggered = 0;
		c = 1;
	}

	if (Pad_[1].TapSpinRight.Triggered) {
		Pad_[1].TapSpinRight.Triggered = 0;
		c = 1;
	}

	if (a)
		b = 0;
}

/* CommonSwitchOnMenuCode__Fv (0x8001a128) */
void CommonSwitchOnMenuCode(void) {
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	GamePaused = 1;
	{
		int line = pYesNoMenu->FindEntry(GameText.pNo);

		pYesNoMenu->SetLine(line);
	}
	Mess_DeleteAll();
	Pad_ActuatorOff(0, 0);
	Pad_ActuatorOff(0, 1);
	SFX_Pause();
	SFX_PlayX(SND_FOOTSLIDE, 4096, 0);

	DAT_800d1e48 = 0;
	Front_OnTime = 0;
}

/* Front_SwitchOnEndHeat__Fv (0x8001a1c8) */
void Front_SwitchOnEndHeat(void) {
	CBruce *s;

	DAT_800d1524 = 0;
	DAT_800d1528 = 0;
	DAT_800d152c = 0;
	s = (CBruce *)MechList->mNextItem;

	if (GVInfo1[0].VideoMode)
		Video_Stop(MechList);

	if (s && GVInfo1[1].VideoMode)
		Video_Stop(s);

	Status = STATUS_ENDHEAT;
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	GamePaused = 1;
	{
		int line = pYesNoMenu->FindEntry(GameText.pNo);

		pYesNoMenu->SetLine(line);
	}
	Mess_DeleteAll();
	Pad_ActuatorOff(0, 0);
	Pad_ActuatorOff(0, 1);
	SFX_Pause();
	SFX_PlayX(SND_FOOTSLIDE, 4096, 0);

	DAT_800d1e48 = 0;
}

/* Front_SwitchOnEndLevelMenu__Fv (0x8001a2bc) */
/* Fuzzy demo twin (0.37; THPS1 drops the checksum debug print); locals from its
   MAIN.SYM. Leaving a level: stop any replay video, pick the end-of-level menu, enter the run's
   score in the level's high-score table (shifting the lower entries down), then set up the menu. */
void Front_SwitchOnEndLevelMenu() {
	CBruce *pSkater;
	CBruce *pSkater2;

	GState = 0;

	if (Status == STATUS_GOTTAPE || Status == STATUS_STATS)
		return;

	pSkater = MechList;
	pSkater2 = (CBruce *)pSkater->mNextItem;

	if (GVInfo1[0].VideoMode)
		Video_Stop(pSkater);

	if (pSkater2 && GVInfo1[1].VideoMode)
		Video_Stop(pSkater2);

	if (GSkater[0]->mNumTapes && GNumberOfPlayers == 1) {
		SFX_PlayX(SND_COLLECT70, 8192, 0);

		Status = STATUS_GOTTAPE;
	} else if (GGame == 2) {
		Status = STATUS_ENDINGSCREEN;
	} else {
		Status = STATUS_STATS;
	}

	if ((GGame == 1 && !Levels[GLevel].isCompetition) || GGame == 3) {
		int i;

		GHighScoreLineIndex = -1;
		GHighScoreInitialCursorPos = 0;

		for (i = 0; i < 8; i++) {
			if (pSkater->mPanel.mTotalScore >= Career_HighScores(GLevel)[i].Score) {
				int j;

				for (j = 7; j > i; j--) {
					Career_HighScores(GLevel)[j].Score = Career_HighScores(GLevel)[j - 1].Score;
					Career_HighScores(GLevel)[j].Skater = Career_HighScores(GLevel)[j - 1].Skater;

					{
						/* the counter is initialised between the two pointers (retail's t0/a3 order) */
						char *d = Career_HighScores(GLevel)[j].Name;
						int k = 3;
						char *s = Career_HighScores(GLevel)[j - 1].Name;

						for (; k != -1; k--)
							*d++ = *s++;
					}
				}

				Career_HighScores(GLevel)[i].Score = pSkater->mPanel.mTotalScore;
				Career_HighScores(GLevel)[i].Skater = GSkater[0]->mSkaterIndex;
				sprintf(Career_HighScores(GLevel)[i].Name, GCareer.Options.LastHighScoreInitials);

				GHighScoreLineIndex = i;
				i = 8;
			}
		}
	}

	if (GGame == 7) {
		DAT_800a5b28[0].init();

		GHorseScale = 0;
		GHorseWaitTime = 45;
		GHorseContinue = 0;
	}

	CommonSwitchOnMenuCode();
	InitStatsSwisch();
}

/* Front_SwitchOnStartLevelMenu__Fv (0x8001a5b8) */
void Front_SwitchOnStartLevelMenu(void) {
	Status = STATUS_STARTHORSE;
	CommonSwitchOnMenuCode();
}

/* Front_PrintHorseLetters__Fv (0x8001a5e0) */
void Front_PrintHorseLetters(void) {
	Status = STATUS_CHANGEHORSEWORD;
	CommonSwitchOnMenuCode();
}

/* Front_SwitchOnVideoScreen__Fv (0x8001a608) */
void Front_SwitchOnVideoScreen() {
	EPausedStatus v;

	if (GVideoRestart.NumSkaters == 0 && Levels[GLevel].isCompetition != 0 && GGame == 1 &&
	    Competition_IsOver() == 0)
		v = STATUS_ENDHEATVIDEOMENU;
	else {
		v = STATUS_VIDEOSCREEN2;

		if (GVideoRestart.NumSkaters == 0)
			v = STATUS_VIDEOSCREEN;
	}

	Status = v;
	CommonSwitchOnMenuCode();
}

/* RestartCompFrig__Fv (0x8001a6a8) */
void RestartCompFrig(void) {
	if (GLevel == 8) {
		int s = RestartNode;

		Trig_SetRestart("RE_START_FRIG");
		Trig_ExecuteRestart();

		RestartNode = s;
	}
}

/* Front_Update__Fv (0x8001a6f8) */
/* Fuzzy demo twin (0.66); locals and blocks from its MAIN.SYM (SoundEffect,
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
   The EntryOn/EntryOff(GameText.pSkipToRestart) calls are each a `do { } while (0)` (macro-style):
   flow weights REG_N_REFS by loop depth, so the loop notes lift the GameText-high gcse
   pseudo's global-alloc priority (25 refs / 177 = 0.565) above the GCareer-high one
   (8 / 45 = 0.533); GameText then skips s0 (which GCareer's prefers), giving retail's s1/s2.
   */
void Front_Update() {
	ESfx SoundEffect = SND_NONE;
	int SwitchOffPauseMode = 0;
	int Activated;
	int GoBack;
	int AnyButton;
	int Start;

	if ((!Pad[0].SpinLeft.Pressed || Status != STATUS_PAUSEDMENU) && Status != STATUS_MEMCARDMENUS)
		Front_GetButtons(Activated, GoBack, AnyButton, Start);

	if (Status)
		Card_CheckStatus(0, 0);

	if ((GSoftResetEnabled && Pad[0].Camera.Pressed && Pad[0].Pause.Pressed) ||
	    (Pad[1].Camera.Pressed && Pad[1].Pause.Pressed)) {
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
			CBruce *pSkater2 = (CBruce *)pSkater->mNextItem;
			SVideoInfo *VInfo = &GVInfo1[0];

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

			if ((Start || !Pad[0].Type || (GNumberOfPlayers == 2 && !Pad[1].Type)) && DemoTimer) {
				GamePaused = 1;
				GSkater[0]->m310 = XFrames;

				if (DAT_800d17cc)
					DAT_800d17cc->m310 = XFrames;

				GSkater[0]->m314 = XFrames;

				if (DAT_800d17cc)
					DAT_800d17cc->m314 = XFrames;

				Mess_DeleteAll();
				Pad_ClearTriggers(&Pad[0]);
				Pad_ClearTriggers(&Pad[1]);

				WobbleAmplitude = 0;
				SFX_Pause();
				SFX_PlayX(SND_FOOTSLIDE, 4096, 0);
				Pad_ActuatorOff(0, 0);
				Pad_ActuatorOff(0, 1);

				pPausedMenu = new CMenu(256, 0, 0, 335, 256, 22);
				pPausedMenu->AddEntry(GameText.pContinue);
				pPausedMenu->AddEntry((Levels[GLevel].isCompetition && GGame == 1)
				                          ? GameText.pRestart
										  : GameText.pRetry);
				pPausedMenu->AddEntry(GameText.pSkipToRestart);

				if (!Cheat_RestartSkip)
					do {
						pPausedMenu->EntryOff(GameText.pSkipToRestart);
					} while (0);

				pPausedMenu->AddEntry(GameText.pChangeSoundLevels);

				if (GGame == 7)
					pPausedMenu->AddEntry(GameText.pEndGame);
				else
					pPausedMenu->AddEntry(GameText.pEndRun);

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

			if (Pad[0].SpinLeft.Pressed) {
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
					pPausedMenu->EntryOn(GameText.pSkipToRestart);
				} while (0);
			else
				do {
					pPausedMenu->EntryOff(GameText.pSkipToRestart);
				} while (0);

			pPausedMenu->Update();

			if (Activated) {
				if (pPausedMenu->ChoiceIs(GameText.pEndRun)) {
					SoundEffect = SND_LAND;
					Game_Over();

					SwitchOffPauseMode = 1;
					GState = 1;
					GSpin = 1;
				}

				if (pPausedMenu->ChoiceIs(GameText.pContinue)) {
					SoundEffect = SND_LAND;
					SwitchOffPauseMode = 1;
				}

				if (pPausedMenu->ChoiceIs((Levels[GLevel].isCompetition && GGame == 1)
				                              ? GameText.pRestart
											  : GameText.pRetry)) {
					SwitchOffPauseMode = 1;
					Horse_Reset();

					EndCode = 7;
					Competition_Init();
				}

				if (pPausedMenu->ChoiceIs(GameText.pSkipToRestart)) {
					SoundEffect = SND_LAND;
					Status = STATUS_SKIPTORESTART;
				}

				if (pPausedMenu->ChoiceIs(GameText.pQuit)) {
					SoundEffect = SND_LAND;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pNo));

					Status = STATUS_REALLYQUIT;
				}

				if (pPausedMenu->ChoiceIs(GameText.pChangeSoundLevels)) {
					SoundEffect = SND_LAND;
					Status = STATUS_SETSOUNDLEVELS;
					PC_StreamResume();
				}

				if (pPausedMenu->ChoiceIs(GameText.pEndGame)) {
					SoundEffect = SND_LAND;
					Front_OnTime = 0;
					GHorseClearInput2 = 0;
					Status = STATUS_ENDINGSCREEN;
				}
			}

			/* Pause resumes (on the PS1, Start activated the highlighted entry) */
			if (GoBack || Start) {
				SoundEffect = SND_LAND;
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

					if (!Utils_CompareStrings(head, "de_", -1))
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
				SFX_PlayX(SND_LAND, 8192, 0);
			}

			if (GoBack) {
				delete pRestartsMenu;
				pRestartsMenu = 0;
				Status = STATUS_PAUSEDMENU;
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
			}

			break;

		case STATUS_REALLYQUIT:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					EndCode = 8;
					SwitchOffPauseMode = 1;
					Horse_Reset();
					SFX_PlayX(SND_LAND, 8192, 0);
				} else {
					Status = STATUS_PAUSEDMENU;
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
				}
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

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
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					SFX_PlayX(SND_LAND, 8192, 0);
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_ENDINGSCREEN);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_ENDLEVELFADESCREEN;
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
				}
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

				Status = STATUS_ENDINGSCREEN;
			}

			break;

		case STATUS_REALLYMAINMENU:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					Status = STATUS_ENDLEVELFADESCREEN;
					SFX_PlayX(SND_LAND, 8192, 0);
				} else {
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

					if (InCompetition_Bool() & !Competition_IsOver_Bool()) {
						Status = STATUS_ENDHEATMENU;
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText.pNextHeat));
					} else {
						Status = STATUS_ENDINGSCREEN;
					}
				}
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

				if (InCompetition_Bool() & !Competition_IsOver_Bool()) {
					Status = STATUS_ENDHEATMENU;
					pEHMenu->SetLine(pEHMenu->FindEntry(GameText.pNextHeat));
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
					SFX_PlayX(SND_LAND, 8192, 0);
					Flash_Fade(16, 1);

					if (pESMenu->ChoiceIs(GameText.pQuit)) {
						if (!Levels[GLevel].isCompetition &&
						    pESMenu->IsEntryOn(GameText.pSaveCareer) && !GLoadedCareerIsSaved) {
							Status = STATUS_QUITSAVE;
							pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));
						} else {
							Status = STATUS_REALLYMAINMENU;
						}
					} else if (pESMenu->ChoiceIs((Levels[GLevel].isCompetition && GGame == 1)
					                                 ? GameText.pRestart
													 : GameText.pRetry)) {
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Competition_Init();
						Horse_Reset();
					} else if (pESMenu->ChoiceIs(GameText.pWatchReplay)) {
						Status = STATUS_VIDEOSCREEN;
					} else if (pESMenu->ChoiceIs(GameText.pSaveCareer) ||
					           pESMenu->ChoiceIs(GameText.pUpdateSaveFile)) {
						pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));

						Status = STATUS_SAVEGAME;
					} else if (pESMenu->ChoiceIs(GameText.pSelectLevel)) {
						StartScreen = 1;
						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pESMenu->ChoiceIs(GameText.pEquipmentSelect)) {
						StartScreen = 2;
						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pESMenu->ChoiceIs(GameText.pLevelHints)) {
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
					SFX_PlayX(SND_LAND, 8192, 0);
					Flash_Fade(32, 1);

					if (pVSMenu->ChoiceIs(GameText.pExit)) {
						Video_Cleanup();

						Status = STATUS_ENDINGSCREEN;
					} else if (pVSMenu->ChoiceIs(GameText.pFullReplay)) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVSMenu->ChoiceIs(GameText.pHighlights)) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVSMenu->ChoiceIs(GameText.pSaveVideo)) {
						Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_VIDEOSCREEN);

						Status = STATUS_MEMCARDMENUS;
					}
				}

				if (GoBack) {
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

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
					SFX_PlayX(SND_LAND, 8192, 0);
					Flash_Fade(32, 1);

					if (pVS2Menu->ChoiceIs(GameText.pExit)) {
						Video_Cleanup();

						Status = STATUS_ENDLEVELFADESCREEN;
					} else if (pVS2Menu->ChoiceIs(GameText.pFullReplay)) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS2Menu->ChoiceIs(GameText.pHighlights)) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					}
				}

				if (GoBack) {
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
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
					SFX_PlayX(SND_LAND, 8192, 0);
					Flash_Fade(32, 1);

					if (pVS3Menu->ChoiceIs(GameText.pReturnToCompetition)) {
						Video_Cleanup();

						Status = STATUS_ENDHEATMENU;
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText.pNextHeat));
					} else if (pVS3Menu->ChoiceIs(GameText.pFullReplay)) {
						GVideoReplay = -1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS3Menu->ChoiceIs(GameText.pHighlights)) {
						GVideoReplay = 1;
						EndCode = 7;
						SwitchOffPauseMode = 1;
						Video_SortAndMerge();
					} else if (pVS3Menu->ChoiceIs(GameText.pSaveVideo)) {
						Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE,
						                       STATUS_ENDHEATVIDEOMENU);

						Status = STATUS_MEMCARDMENUS;
					}
				}

				if (GoBack) {
					SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

					Status = STATUS_ENDHEATMENU;
					pEHMenu->SetLine(pEHMenu->FindEntry(GameText.pNextHeat));
				}
			} else {
				Front_OnTime++;
			}

			break;

		case STATUS_GOTTAPE:
			if (Front_OnTime > 10) {
				if (Activated && CTape::DisplayContinueButton()) {
					SFX_PlayX(SND_LAND, 8192, 0);

					{
						int i;

						for (i = 0; i < 5; i++) {
							delete GTape[i];
							GTape[i] = 0;
						}
					}

					Status = STATUS_TAPESAVE;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pYes));
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

			if (GHorseCursorPos >= GCareer.Options.HorseLen - 1 && GCareer.Options.HorseLen >= 4) {
				lastAlpha = ' ';
				firstAlpha = ' ';
			}

			if (UpDown == -1) {
				SizeUp = 16;
				SFX_PlayX(SND_COUNTDING, 8192, 0);

				if (GCareer.Options.HorseWord[GHorseCursorPos] == ' ' ||
				    GCareer.Options.HorseWord[GHorseCursorPos] == 0) {
					GCareer.Options.HorseWord[GHorseCursorPos] = 'A';
				} else {
					GCareer.Options.HorseWord[GHorseCursorPos]++;

					if ((uchar)GCareer.Options.HorseWord[GHorseCursorPos] > 'Z')
						GCareer.Options.HorseWord[GHorseCursorPos] = firstAlpha;
				}
			}

			if (UpDown == 1) {
				SizeUp = 16;
				SFX_PlayX(SND_COUNTDING, 8192, 0);

				if (GCareer.Options.HorseWord[GHorseCursorPos] == ' ' ||
				    GCareer.Options.HorseWord[GHorseCursorPos] == 0) {
					GCareer.Options.HorseWord[GHorseCursorPos] = 'Z';
				} else {
					GCareer.Options.HorseWord[GHorseCursorPos]--;

					if ((uchar)GCareer.Options.HorseWord[GHorseCursorPos] < 'A')
						GCareer.Options.HorseWord[GHorseCursorPos] = lastAlpha;
				}
			}

			GCareer.Options.HorseLen = 10;

			{
				int i;

				for (i = 0; i < 10; i++) {
					if (GCareer.Options.HorseWord[i] == ' ' || GCareer.Options.HorseWord[i] == 0) {
						GCareer.Options.HorseLen = i;
						break;
					}
				}
			}

			if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered) {
				SizeUp = 16;
				SFX_PlayX(SND_LAND, 8192, 0);

				Pad[0].MenuLeft.Triggered = 0;
				Pad[1].MenuLeft.Triggered = 0;
				GHorseCursorPos--;

				if (GHorseCursorPos < 0)
					GHorseCursorPos = GCareer.Options.HorseLen;

				if (GHorseCursorPos >= 10)
					GHorseCursorPos = 9;
			}

			if (Pad[0].MenuRight.Triggered || Pad[1].MenuRight.Triggered) {
				SizeUp = 16;
				SFX_PlayX(SND_LAND, 8192, 0);

				Pad[0].MenuRight.Triggered = 0;
				Pad[1].MenuRight.Triggered = 0;
				GHorseCursorPos++;

				if (GHorseCursorPos > GCareer.Options.HorseLen)
					GHorseCursorPos = 0;

				if (GHorseCursorPos >= 10)
					GHorseCursorPos = 0;
			}

			if (Activated) {
				{
					int i;

					for (i = 0; i < 10; i++) {
						if (GCareer.Options.HorseWord[i] == ' ') {
							GCareer.Options.HorseWord[i] = 0;
							break;
						}
					}
				}

				sprintf(GPreferences.HorseWord, GCareer.Options.HorseWord);

				GHorseWordChosen = 1;
				GHorseDoneStart = 0;
				GPreferences.HorseLen = GCareer.Options.HorseLen;
				SFX_PlayX(SND_LAND, 8192, 0);
			}
		} break;

		case STATUS_STARTHORSE:
			GHorseClearInput2 = 1;
			GSkater[0]->m310 = XFrames;

			if (Activated)
				SwitchOffPauseMode = 1;

			break;

		case STATUS_TAPESAVE:
			pYesNoMenu->Update();

			if (Activated) {
				SFX_PlayX(SND_LAND, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_STATS);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_STATS;
				}
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

				Status = STATUS_GOTTAPE;
			}

			break;

		case STATUS_HIGHSCORE:
			if (Front_OnTime > 10) {
				if (GHighScoreLineIndex != -1) {
					int UpDown = GetUpDown();

					if (UpDown == -1) {
						SizeUp = 16;
						SFX_PlayX(SND_COUNTDING, 8192, 0);

						if (Career_HighScores(GLevel)[GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == ' ' ||
						    Career_HighScores(GLevel)[GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == 0) {
							Career_HighScores(GLevel)[GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos] = 'A';
						} else {
							Career_HighScores(GLevel)[GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos]++;

							if ((uchar)Career_HighScores(GLevel)[GHighScoreLineIndex]
							        .Name[GHighScoreInitialCursorPos] > 'Z')
								Career_HighScores(GLevel)[GHighScoreLineIndex]
								    .Name[GHighScoreInitialCursorPos] = ' ';
						}
					}

					if (UpDown == 1) {
						SizeUp = 16;
						SFX_PlayX(SND_COUNTDING, 8192, 0);

						if (Career_HighScores(GLevel)[GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == ' ' ||
						    Career_HighScores(GLevel)[GHighScoreLineIndex]
						            .Name[GHighScoreInitialCursorPos] == 0) {
							Career_HighScores(GLevel)[GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos] = 'Z';
						} else {
							Career_HighScores(GLevel)[GHighScoreLineIndex]
							    .Name[GHighScoreInitialCursorPos]--;

							if ((uchar)Career_HighScores(GLevel)[GHighScoreLineIndex]
							        .Name[GHighScoreInitialCursorPos] < 'A')
								Career_HighScores(GLevel)[GHighScoreLineIndex]
								    .Name[GHighScoreInitialCursorPos] = ' ';
						}
					}

					if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered) {
						SizeUp = 16;
						SFX_PlayX(SND_LAND, 8192, 0);

						Pad[0].MenuLeft.Triggered = 0;
						Pad[1].MenuLeft.Triggered = 0;
						GHighScoreInitialCursorPos--;

						if (GHighScoreInitialCursorPos < 0)
							GHighScoreInitialCursorPos = 2;
					}

					if (Pad[0].MenuRight.Triggered || Pad[1].MenuRight.Triggered) {
						SizeUp = 16;
						SFX_PlayX(SND_LAND, 8192, 0);

						Pad[0].MenuRight.Triggered = 0;
						Pad[1].MenuRight.Triggered = 0;
						GHighScoreInitialCursorPos++;

						if (GHighScoreInitialCursorPos >= 3)
							GHighScoreInitialCursorPos = 0;
					}
				}

				if (Activated) {
					SFX_PlayX(SND_COLLECT56, 8192, 0);
					sprintf(GCareer.Options.LastHighScoreInitials,
					        Career_HighScores(GLevel)[GHighScoreLineIndex].Name);

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
					SFX_PlayX(SND_LAND, 8192, 0);

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
							GHorseLetters[0] = NextHorseLetters[0];
							GHorseLetters[1] = NextHorseLetters[1];
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
					SFX_PlayX(SND_LAND, 8192, 0);
					Flash_Fade(32, 1);

					if (pEHMenu->ChoiceIs(GameText.pQuit)) {
						Status = STATUS_REALLYMAINMENU;
					} else if (pEHMenu->ChoiceIs(GameText.pNextHeat)) {
						SwitchOffPauseMode = 1;
						EndCode = 7;
					} else if (pEHMenu->ChoiceIs(GameText.pEndComp)) {
						Status = STATUS_ENDHEATWINLOSE;
					} else if (pEHMenu->ChoiceIs(GameText.pWatchReplay)) {
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
				SFX_PlayX(SND_LAND, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
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
						pEHMenu->SetLine(pEHMenu->FindEntry(GameText.pNextHeat));
					} else {
						Status = STATUS_ENDHEATWINLOSE;
					}
				} else {
					Competition_FullyDisplayLadder();
					SFX_PlayX(SND_LEADER5, 8192, 0);
				}
			}

			break;

		case STATUS_ENDHEATREPLAY:
			Competition_JudgeLogic();

			if (Front_OnTime > 10) {
				if (Activated) {
					if (Competition_JudgeFullyDisplayed()) {
						SFX_PlayX(SND_LAND, 8192, 0);

						Status = STATUS_ENDHEATLADDER;
						GVideoReplay = -1;
						RestartCompFrig();
						delete pPausedMenu;
						pPausedMenu = 0;

						if (EndCode != 8 && EndCode != 4 && EndCode != 3)
							SFX_Unpause();

						Video_SortAndMerge();

						if (GVInfo1[0].NumFrames > 90) {
							CBruce *pSkater = MechList;
							CBruce *pSkater2 = (CBruce *)pSkater->mNextItem;

							GamePaused = 0;
							GVideoReplay = 0;
							GVInfo1[0].ReplayHighlight = 0;
							GVInfo1[1].ReplayHighlight = 0;
							Video_Start(pSkater, VIDEO_PLAYBACK, 1, 0x7fffffff);

							if (pSkater2)
								Video_Start(pSkater2, VIDEO_PLAYBACK, 1, 0x7fffffff);

							GState = 0;
							Flash_Reset();

							((CBruce *)MechList)->mTime = Xblanks + 216030;

							if (MechList->mNextItem)
								((CBruce *)MechList->mNextItem)->mTime = Xblanks + 216030;
						}
					} else {
						Competition_FullyDisplayJudge();
						SFX_PlayX(SND_LEADER5, 8192, 0);
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
				SFX_PlayX(SND_LAND, 8192, 0);
				Flash_Fade(32, 1);

				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					Front_SetSaveAction(SAVE_GAME);
					Front_InitMemCardMenus(512, SVSTATUS_CHECKINGCARDPAUSE, STATUS_ENDINGSCREEN);

					Status = STATUS_MEMCARDMENUS;
				} else {
					Status = STATUS_ENDINGSCREEN;
				}
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

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

			XALEVEL = (pSoundMenu->ChoiceIs(musicBuf) || pSoundMenu->ChoiceIs(GameText.pSkipTrack))
			              ? GPreferences.MusicVolume * 0x555
			              : 0;

			if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered) {
				Pad[0].MenuLeft.Triggered = 0;
				Pad[1].MenuLeft.Triggered = 0;

				if (pSoundMenu->ChoiceIs(soundBuf)) {
					if (soundVol > 0)
						soundVol--;

					GPreferences.SFXVolume = GCareer.Options.SFXVolume = soundVol * 6 / 5;
					sprintf(soundBuf, GameText.pSFXLevel, soundVol);

					SFXLEVEL = GCareer.Options.SFXVolume * 0x555;
					SFX_PlayX(SND_LAND, 8192, 0);
				} else if (pSoundMenu->ChoiceIs(musicBuf)) {
					if (musicVol > 0)
						musicVol--;

					GPreferences.MusicVolume = GCareer.Options.MusicVolume = musicVol * 6 / 5;
					sprintf(musicBuf, GameText.pMusicLevel, musicVol);
				}
			}

			if (Pad[0].MenuRight.Triggered || Pad[1].MenuRight.Triggered) {
				Pad[0].MenuRight.Triggered = 0;
				Pad[1].MenuRight.Triggered = 0;

				if (pSoundMenu->ChoiceIs(soundBuf)) {
					if (soundVol < 10)
						soundVol++;

					GPreferences.SFXVolume = GCareer.Options.SFXVolume = soundVol * 6 / 5;
					sprintf(soundBuf, GameText.pSFXLevel, soundVol);

					SFXLEVEL = GCareer.Options.SFXVolume * 0x555;
					SFX_PlayX(SND_LAND, 8192, 0);
				} else if (pSoundMenu->ChoiceIs(musicBuf)) {
					if (musicVol < 10)
						musicVol++;

					GPreferences.MusicVolume = GCareer.Options.MusicVolume = musicVol * 6 / 5;
					sprintf(musicBuf, GameText.pMusicLevel, musicVol);
				}
			}

			if (Activated) {
				if (pSoundMenu->ChoiceIs(GameText.pSkipTrack))
					GameFMV_SkipTrack();
				else if (pSoundMenu->ChoiceIs(GameText.pBackToMainMenu))
					GoBack = 1;
			}

			if (GoBack) {
				SFX_PlayX(SND_FOOTSLIDE, 8192, 0);

				Status = STATUS_PAUSEDMENU;
				XALEVEL = GCareer.Options.MusicVolume * 0x555;
				PC_StreamPause();
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

/* Front_FindLevel__FPc (0x8001cb30) */
SLevel *Front_FindLevel(char *s) {
	SLevel *r = 0;
	int i = 0;

	while (*Levels[i].pTRGName) {
		if (Utils_CompareStrings(Levels[i].pTRGName, s, 0xffffffff)) {
			CurrentGameLevel = i;
			r = &Levels[i];
			break;
		}

		if (++i >= 41)
			break;
	}

	return r;
}

/* Front_SaveGameState__Fv (0x8001cbf0) */
void Front_SaveGameState(void) {
	CVector v;
	char *s = TrigFileName, *d = LastGameState.State.TrigFileName;

	while (*s)
		*d++ = *s++;

	*d = 0;
	s = (char *)Trig_GetPosition(v, RestartNode) + 6;
	d = LastGameState.State.RestartName;

	while (*s)
		*d++ = *s++;

	*d = 0;

	if (MechList) {
		LastGameState.State.Health = MechList->mHealth;
		LastGameState.State.WeaponsGot = 0;
	}

	Front_SavePreferences(LastGameState.Preferences);

	LastGameState.Checksum = Front_CalculateGameChecksum(LastGameState);
}

/* Front_CalculateGameChecksum__FR9SSaveGame (0x8001cccc) */
uint Front_CalculateGameChecksum(SSaveGame &game) {
	uint *g = (uint *)&game;
	unsigned c = 0;
	unsigned i;

	for (i = 1; i < 34; i++)
		c ^= g[i];

	return c | 1;
}

/* Front_LoadGame__FP9SSaveGame (0x8001ccf8) */
void Front_LoadGame(SSaveGame *p) {
	int s2, t1, t2, u1, u2, i1;
	SStats *q;

	TrigFileName = p->State.TrigFileName;
	Utils_InitialRand(0x12a6cc58);
	FileIO_SetSectorsPerFrame(2000);
	SFX_Init("skate2.vab");
	FUN_8004b0e4(Levels[GLevel].pVab);
	SFX_SetReverbType(0);

	pScoreSprite = new Sprite("scorep.bmp", 1, 256, 256);
	pSpecialSprite = new Sprite("special.bmp", 1, 256, 256);
	pBarSprite = new Sprite("spec_bar.bmp", 1, 256, 256);
	DAT_800d1858 = new Sprite("paused.bmp", 1, 64, 256);
	i1 = Spool_PSX("bits", 0, 0);
	PSXRegion[i1].Protected = 1;
	s2 = Spool_PSX("items", 0, 0);
	PSXRegion[s2].Protected = 1;
	CurrentItemsFileRegion = s2;
	Spool_Sync();
	FileIO_SetSectorsPerFrame(4);
	FUN_8006c4c4();
	Font_Init();
	Bit_Init();
	Spool_ClearAllPSXs();

	if (Levels[GLevel].isCompetition && GGame == 1)
		s2 = Spool_PSX("SkMedals", 0, 0);

	CurrentMedalsFileRegion = s2;

	if (Cheat_BigHead) {
		q = &Stats[SelectedSkater];
		t1 = q->Head;
		t2 = q->Neck;
		GHeadPart = t1;
		GHeadUp = t2;
	}

	if (GNumberOfPlayers == 2 && GGame != 7)
		Spool_PSX(Stats[SelectedSkater].PSXName2, 0, 0);
	else
		Spool_PSX(Stats[SelectedSkater].PSXName, 0, 0);

	Spool_Sync();

	GHeadPart = -1;

	if (GNumberOfPlayers == 2) {
		if (Cheat_BigHead) {
			q = &Stats[SelectedSkater2];
			u1 = q->Neck;
			u2 = q->Head;
			GHeadUp = u1;
			GHeadPart = u2;
		}

		if (GGame != 7)
			Spool_PSX(Stats[SelectedSkater2].PSXName2, 0, 0);
		else
			Spool_PSX(Stats[SelectedSkater2].PSXName, 0, 0);

		Spool_Sync();

		GHeadPart = -1;
	}

	if (Front_FindLevel(TrigFileName)) {
		FUN_8004bb5c();
		FUN_8004b180();
	}

	FileIO_SetSectorsPerFrame(2000);
	Trig_InitialParseTRGFile(TrigFileName);
	FUN_8005dc8c();
	FileIO_SetSectorsPerFrame(4);

	if (BackOut == 0) {
		CreateSkatersAndCameras();

		if (p->State.RestartName[0])
			Trig_SetRestart(p->State.RestartName);

		Trig_ExecuteRestart();
		Front_SaveGameState();
	}
}

/* the static constructor/teardown of GHorseFallingText: named as retail's (dots and all), as
   game/data's GameCtorList names them (PSX_NAME there), so the build places them by that name.
   (FRONT_PSX_NAME: see Pad_) */
extern "C" void GLOBAL_D_LoadingScreen_Main() FRONT_PSX_NAME("_GLOBAL_.D.LoadingScreen_Main__Fv");
extern "C" void GLOBAL_I_LoadingScreen_Main() FRONT_PSX_NAME("_GLOBAL_.I.LoadingScreen_Main__Fv");

/* _GLOBAL_.D.LoadingScreen_Main__Fv (0x8001d1d0) */
void GLOBAL_D_LoadingScreen_Main() {
	FUN_8001dfec(DAT_800a5b28, 2); /* ~FallingTextEffect */
}

/* _GLOBAL_.I.LoadingScreen_Main__Fv (0x8001d1f8) */
void GLOBAL_I_LoadingScreen_Main() {
	__17FallingTextEffect(DAT_800a5b28); /* FallingTextEffect() */
}

/* FUN_8001d21c (0x8001d21c) */
void FUN_8001d21c(char *p, int f) {
	*(char **)(p + 0) = DAT_8009e548;

	if (f & 1)
		__dl__6CClassPv(p);
}
