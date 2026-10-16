/* RFRONT.CPP: the front end's memory-card screens (low- and hi-res display, the save/load state
   machine Front_MemCardUpdate), text over cached backgrounds, and entering a save name
   (declarations in game/rfront.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/rfront.h"
#include "game/padbuttons.h"
#include "game/mess.h"
#include "game/shellutils.h"
#include "game/front.h"
#include "game/psyq.h"
#include "game/hires.h"
#include "game/card.h"
#include "game/career.h"
#include "game/video.h"
#include "game/pad.h"
#include "game/mem.h"
#include "game/sfx.h"
#include "game/competition.h"
#include "game/globals.h"
#include "game/main.h"
#include "pc_saves.h"

/* a polymorphic object as Front_FreeMemCardMenus deletes it: through its egcs vtable's
   deleting-destructor entry (slot 0: this adjustment, then the function) */
struct SFrontVtable {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};
struct SFrontVObject {
	SFrontVtable *vt;
};

/* the 3-byte template ("A" and padding) Front_MemCardDisplay copies a save-name letter into */
struct SFrontLetterText {
	char a, b, c;
};

/* A C name for a declaration (as game/data's PSX_NAME): `__asm`, not `__asm__`, which portsrc's
   inline-asm translation would take for MIPS code. */
#define RFRONT_STR_(x) #x
#define RFRONT_STR(x) RFRONT_STR_(x)
#define RFRONT_PSX_NAME(name) __asm(RFRONT_STR(__USER_LABEL_PREFIX__) name)

extern SFrontVObject *Tri1;
extern SFrontVObject *Tri2;
/* Pad again, without its count, for GetUpDown: its code was built against `uchar Pad[]`, and gcc
   merges the two pads' tests when it knows both are there */
extern SControl Pad_[] RFRONT_PSX_NAME("Pad");
extern int LeftRightAutoRepeatTimer;
extern SCachedBackground BGCache[10];
extern int FreeSprite;
extern char DAT_800d1138[];
extern char s_800a42dc[];
extern char s_800a42e8[];
extern char ASCIIName[][11];
extern char *VertMenuItem[];
extern int VertMenuItemIndex[];
extern char msg[];
extern int DAT_800d113c;
extern int DAT_800d1140;
extern int VideoToDisk;
extern int CursorPos;
extern int NumVertMenuItems, VertMenuTopIndex, VertMenuTopChanged, MCState, OldMCState, MCResolution;
extern int CheckingCardPause;
extern int GLoadAction;
extern int Delay, Retries, SaveNeedBlocks, SaveFreeBlocks, GReturnToState;


void RGBRed();
void Front_DirtyRectText(int, int, const char *);
int SaveReturnState();

/* Front_FreeMemCardMenus__Fv (0x80011150) */
void Front_FreeMemCardMenus(void) {
	SFrontVObject *o = Tri1;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	o = Tri2;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	o = (SFrontVObject *)pVertMenu;
	Tri2 = 0;
	Tri1 = 0;

	if (o)
		o->vt->fn((char *)o + o->vt->off, 3);

	pVertMenu = 0;
}

/* RGBRed__Fv (0x800111e8) */
void RGBRed(void) {
	Mess_SetRGB(96, 96, 96, 0);
}

/* RGBGreen__Fv (0x80011214) */
void RGBGreen(void) {
	Mess_SetRGB(96, 96, 96, 0);
}

/* GetUpDown__Fv (0x80011240) */
int GetUpDown(void) {
	int r = 0;

	if (Pad_[0].MenuUp.Pressed || Pad_[1].MenuUp.Pressed || Pad_[0].MenuDown.Pressed ||
	    Pad_[1].MenuDown.Pressed) {
		if (LeftRightAutoRepeatTimer == 0 ||
		    (LeftRightAutoRepeatTimer >= 16 && (LeftRightAutoRepeatTimer & 3) == 0)) {
			if (Pad_[0].MenuDown.Pressed || Pad_[1].MenuDown.Pressed)
				r = 1;

			if (Pad_[0].MenuUp.Pressed || Pad_[1].MenuUp.Pressed)
				r = -1;
		}

		LeftRightAutoRepeatTimer++;
	} else {
		LeftRightAutoRepeatTimer = 0;
	}

	return r;
}

/* Front_DirtyRectText__FiiPCc (0x80011324) */
/* 0x8001ddb0 */
void Front_DirtyRectText(int x, int y, const char *s) {
	int i, w, h, nx;
	SCachedBackground *e;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].pString == s) {
			Mess_DrawText(x, y, s, 0, 0, 256, 0);
			return;
		}
	}

	BGCache[FreeSprite].pString = s;
	w = Mess_TextWidth(s);
	h = Mess_TextHeight(s);
	nx = x - (w >> 1);
	w += 4;
	e = &BGCache[FreeSprite];
	e->x = nx;
	e->y = y - h;
	h += 4;
	e->pSprite = new Sprite(nx, BGCache[FreeSprite].y, w, h, 1, 32, 128);
	BGCache[FreeSprite].State = 2;
	FreeSprite++;
}

/* Front_RestoreBackground__Fv (0x800114a8) */
void Front_RestoreBackground() {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].State == 2)
			BGCache[i].pSprite->draw(BGCache[i].x, BGCache[i].y, 0, -1, -1, -1);
	}
}

/* Front_FreeBackground__Fv (0x80011570) */
void Front_FreeBackground() {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		delete BGCache[i].pSprite;
		BGCache[i].State = 0;
	}

	FreeSprite = 0;
}

/* Front_InitMemCardMenus__Fi13EMemCardState13EPausedStatus (0x8001160c) */
void Front_InitMemCardMenus(int a, EMemCardState b, EPausedStatus c) {
	GReturnToState = c;
	FreeSprite = 0;
	MCResolution = a;
	MCState = b;
	CheckingCardPause = 0;
}

/* Front_SetLoadAction__F11ELoadAction (0x80011628) */
void Front_SetLoadAction(ELoadAction v) {
	GLoadAction = v;
}

/* Front_SetSaveAction__F11ESaveAction (0x80011634) */
void Front_SetSaveAction(ESaveAction v) {
	GSaveAction = v;
}

/* Front_ExtractASCIINameFromSJIS__FPcT0 (0x80011640) */
void Front_ExtractASCIINameFromSJIS(char *s, char *d) {
	int i;

	for (i = 0; i < 10; i++) {
		if (*s == 0) {
			*d = 0;
			return;
		}

		if (*s == 0x82) {
			*d++ = s[1] - 31;
			s += 2;
		} else if (*s == 0x81) {
			*d++ = ' ';
			s += 2;
		}
	}
}

/* Front_SetSJISName__FPc (0x800116b4) */
/* Demo twin; locals from its MAIN.SYM. Converts the 10-character high-score name
   entry buffer to Shift-JIS for the memory card title: trailing blanks are trimmed, embedded holes
   become spaces, A-Z map to full-width letters (0x82, c+31) and spaces to 0x81 0x40. */
void Front_SetSJISName(char *SJISName) {
	int i;

	for (i = 9; i > 0; i--) {
		if (SaveGameName[i] != ' ' && SaveGameName[i])
			break;

		SaveGameName[i] = 0;
	}

	for (; i >= 0; i--) {
		if (!SaveGameName[i])
			SaveGameName[i] = ' ';
	}

	for (i = 0; i < 10; i++) {
		if (!SaveGameName[i]) {
			*SJISName = 0;
			return;
		}

		if ((unsigned char)(SaveGameName[i] - 'A') < 26) {
			*SJISName++ = 130;
			*SJISName++ = SaveGameName[i] + 31;
		} else if (SaveGameName[i] == ' ') {
			*SJISName++ = 129;
			*SJISName++ = 64;
		}
	}
}

/* InitEnterName__Fv (0x800117b4) */
void InitEnterName(void) {
	CursorPos = 0;
	SizeUp = 0;
	Pad[0].MenuUp.Triggered = 0;
	Pad[0].MenuDown.Triggered = 0;
	Pad[1].MenuUp.Triggered = 0;
	Pad[1].MenuDown.Triggered = 0;

	if (GSaveAction == 0)
		strcpy(SaveGameName, s_800a42e8);
	else
		strcpy(SaveGameName, s_800a42dc);
}

/* Front_MemCardDisplay__Fv (0x80011820) */
/* Memory-card front-end screen (RFRONT.CPP). The save-slot list loop follows the demo twin:
   x is XMid - 150 + i * 30 (no separate column variable), and the slot letter goes into a copy of a
   3-byte template in buf. */
void Front_MemCardDisplay() {
	int s2, s0, s1, t;
	char buf[24];
	unsigned char c;
	CMenu *o;
	char *a;

	s2 = MCResolution >> 1;
	Mess_SetScale(256);
	Mess_SetTextJustify(0);

	switch ((unsigned)MCState) {
		case 0:
		case 1:
		case 15:
		case 16:
			RGBGreen();
			break;
		case 9:
		case 17:
			RGBRed();
			Mess_DrawText(s2, 90, GameText.pPleaseInsert1, 0, 0, 256, 0);
			Mess_DrawText(s2, 116, GameText.pPleaseInsert2, 0, 0, 256, 0);
			Mess_DrawText(s2, 142, GameText.pPleaseInsert3, 0, 0, 256, 0);
			Mess_DrawText(s2, 168, GameText.pPleaseInsert4, 0, 0, 256, 0);
			break;
		case 10:
			Front_PrintGoBackSelect();
			RGBRed();
			Mess_DrawText(s2, 35, GameText.pUnformatted1, 0, 0, 256, 0);
			Mess_DrawText(s2, 61, GameText.pUnformatted2, 0, 0, 256, 0);
			Mess_DrawText(s2, 87, GameText.pUnformatted3, 0, 0, 256, 0);
			Mess_DrawText(s2, 113, GameText.pUnformatted4, 0, 0, 256, 0);

			o = pYesNoMenu;
			o->mY = 148;
			o->Display();
			break;
		case 11:
			RGBGreen();

			if ((TTime / 10) & 1)
				Mess_DrawText(s2, 110, GameText.pFormatting, 0, 0, 256, 0);

			break;
		case 13:
			RGBGreen();
			Mess_DrawText(s2, 110, GameText.pFormatComplete, 0, 0, 256, 0);
			break;
		case 12:
			RGBRed();
			Mess_DrawText(s2, 110, GameText.pFormatFailed, 0, 0, 256, 0);
			break;
		case 8:
			RGBRed();
			Mess_SetScale(192);
			Mess_DrawText(s2, 67, GameText.pCardFull1, 0, 0, 256, 0);
			Mess_DrawText(s2, 85, GameText.pCardFull2, 0, 0, 256, 0);
			Mess_DrawText(s2, 103, GameText.pCardFull3, 0, 0, 256, 0);
			sprintf(buf, GameText.pCardFull4, SaveNeedBlocks, SaveNeedBlocks == 1 ? "" : "s");
			Mess_DrawText(s2, 121, buf, 0, 0, 256, 0);
			sprintf(buf, GameText.pCardFull5, SaveNeedBlocks - SaveFreeBlocks,
			        SaveNeedBlocks - SaveFreeBlocks == 1 ? "" : "s");
			Mess_DrawText(s2, 139, buf, 0, 0, 256, 0);
			Mess_DrawText(s2, 157, GameText.pCardFull6, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.pContinue);
			break;
		case 3:
			RGBGreen();
			Mess_DrawText(s2, 50, GameText.pAFileByThat, 0, 0, 256, 0);
			Mess_DrawText(s2, 76, GameText.pNameAlreadyExists, 0, 0, 256, 0);
			Mess_DrawText(s2, 102, GameText.pOKToOverwrite1, 0, 0, 256, 0);
			Front_PrintGoBackSelect();

			o = pYesNoMenu;
			o->mY = 145;
			o->Display();
			break;
		case 4:
			RGBGreen();
			Mess_DrawText(s2, 110, GameText.pSaving, 0, 0, 256, 0);
			break;
		case 7:
			RGBGreen();
			Mess_DrawText(s2, 110, GameText.pSaveComplete, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.pContinue);
			break;
		case 6:
			RGBRed();
			Mess_DrawText(s2, 110, GameText.pSaveFailed, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.pContinue);
			break;
		case 2:
			s0 = 0;
			RGBRed();
			Mess_SetTextJustify(0);
			Mess_SetScale(256);
			Mess_DrawText(s2, 40, GameText.pEnterName, 0, 0, 256, 0);

			do {
				if (SizeUp > 0)
					SizeUp--;

				Mess_SetScale(256);

				if (s0 == CursorPos)
					Mess_SetRGB(182, 132, 36, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				Mess_DrawText(s2 - 150 + s0 * 30, 100, "_", 0, 0, 256, 0);

				if (s0 == CursorPos)
					Mess_SetScale(SizeUp * 6 + 256);

				if (SaveGameName[s0]) {
					*(SFrontLetterText *)buf = *(SFrontLetterText *)DAT_800d1138;
					buf[0] = SaveGameName[s0];
					Mess_DrawText(s2 - 150 + s0 * 30, 92, buf, 0, 0, 256, 0);
				}

				s0++;
			} while (s0 < 10);

			Front_PrintGoBackSelect();
			break;
		case 14:
			Mess_DrawText(s2, 100, "no space do you want to delete blah blah", 0, 0, 256, 0);
			break;
		case 19:
			RGBRed();

			if (GLoadAction == 2)
				Mess_DrawText(s2, 110, GameText.pNoCareerFiles, 0, 0, 256, 0);
			else
				Mess_DrawText(s2, 110, GameText.pNoVideoFiles, 0, 0, 256, 0);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 20:
			if (MCResolution == 512)
				Front_PrintGoBackSelect();

			break;
		case 21:
			RGBGreen();
			Mess_DrawText(s2, 110, "loading video file", 0, 0, 256, 0);
			break;
		case 22:
			RGBRed();
			Mess_DrawText(s2, 110, "load failed", 0, 0, 256, 0);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 23:
			break;
	}
}

/* Front_MemCardHiResDisplay__Fv (0x8001206c) */
/* From the demo twin + its MAIN.SYM locals (i, XMid, block `load`, block `up`).
   Hi-res memory-card screen: the save/load progress screen, the background state machine, then
   one case per card state (case 20 = the save-file VerticalMenu with scroll arrows).
   Levers: the arrow text is `if (up && down) ... else sprintf(.., up ? a : b)` with `down` a
   separate block local (keeps up in a1 and XMid in s1); DrawSync returns int; the
   VerticalMenu ?: count is `n < 6 ? n : 5`; sizeof(VerticalMenu) = 820. */
void Front_MemCardHiResDisplay() {
	int i;
	int XMid;

	Mess_SetScale(200);

	XMid = MCResolution >> 1;

	if (MCState == 21 && GLoadAction == 2) {
		{
			char *load;

			for (i = 0; i < 2; i++) {
				HiRes_ClearOTag();
				ResetOT();

				if (pVertMenu)
					pVertMenu->erase();

				HiRes_DrawOTag();
			}

			load = "loading saved game";
			DrawSync(0);

			for (i = 0; i < 4; i++) {
				HiRes_ClearOTag();
				ResetOT();
				RGBGreen();
				Front_DirtyRectText(XMid, 240, load);
				RGBRed();
				HiRes_DrawOTag();
			}
		}

		OldMCState = 24;
		return;
	}

	if (OldMCState != 26)
		Front_RestoreBackground();

	{
		int w = OldMCState;
		int v = w;

		if (v != MCState) {
			if (v == 15)
				goto set;

			if (v == 24) {
				OldMCState = 25;
				return;
			}

			if (v == 25) {
				OldMCState = 26;
				return;
			}

			if (w != 26) {
				OldMCState = 24;
				return;
			}

			Front_FreeBackground();
		set:
			OldMCState = MCState;
		}
	}

	Mess_SetScale(200);
	Mess_SetTextJustify(0);

	switch (MCState) {
		case 15:
		case 16:
			RGBGreen();
			Front_DirtyRectText(XMid, 180, GameText.pCheckingCard);
			RGBRed();
			Front_DirtyRectText(XMid, 280, GameText.pDoNotRemove1);
			Front_DirtyRectText(XMid, 332, GameText.pDoNotRemove2);
			break;
		case 17:
			RGBRed();
			Front_DirtyRectText(XMid, 180, GameText.pPleaseInsert1);
			Front_DirtyRectText(XMid, 232, GameText.pPleaseInsert2);
			Front_DirtyRectText(XMid, 284, GameText.pPleaseInsert3);
			Front_DirtyRectText(XMid, 336, GameText.pPleaseInsert4);
			break;
		case 18:
			RGBRed();
			Front_DirtyRectText(XMid, 174, GameText.pHiResUnformatted1);
			Front_DirtyRectText(XMid, 226, GameText.pHiResUnformatted2);
			Front_DirtyRectText(XMid, 278, GameText.pHiResUnformatted3);
			break;
		case 19:
			RGBRed();

			if (GLoadAction == 2)
				Front_DirtyRectText(XMid, 220, GameText.pNoCareerFiles);
			else
				Front_DirtyRectText(XMid, 220, GameText.pNoVideoFiles);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 20:
			if (!pVertMenu) {
				pVertMenu = new VerticalMenu(100, 398, 15, VertMenuItem,
				                             NumVertMenuItems < 6 ? NumVertMenuItems : 5, 0, 0, 0);
				sprintf(msg, GameText.pHiResScroll, GameText.pHiResUpOrDown);
				Front_DirtyRectText(XMid, 380, msg);
			}

			for (i = 0; i < 5; i++) {
				if (i != pVertMenu->getSelected() || VertMenuTopChanged)
					pVertMenu->setString(i, VertMenuItem[VertMenuTopIndex + i]);
			}

			if (NumVertMenuItems > 5 && ((TTime / 20) & 3)) {
				RGBGreen();

				{
					int up = VertMenuTopIndex > 0;
					int down = VertMenuTopIndex < NumVertMenuItems - 5;

					if (up && down)
						sprintf(msg, GameText.pHiResScroll, GameText.pHiResUpOrDown);
					else
						sprintf(msg, GameText.pHiResScroll,
						        up ? GameText.pHiResUp : GameText.pHiResDown);
				}

				Front_DirtyRectText(XMid, 380, msg);
			}

			pVertMenu->draw();

			if (MCResolution == 512)
				Front_PrintGoBackSelect();

			break;
		case 21:
			RGBGreen();
			Front_DirtyRectText(XMid, 220, "loading video file");
			RGBRed();
			Front_DirtyRectText(XMid, 280, GameText.pDoNotRemove1);
			Front_DirtyRectText(XMid, 332, GameText.pDoNotRemove2);
			break;
		case 22:
			RGBRed();
			Front_DirtyRectText(XMid, 220, GameText.pCardError);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 23:
			RGBRed();
			Front_DirtyRectText(XMid, 220, "Video loaded");

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.pContinue);

			break;
		case 27:
			break;
	}
}

/* NameWasBlank__Fv (0x80012600) */
int NameWasBlank(void) {
	int i;

	for (i = 0; i < 10; i++) {
		if (SaveGameName[i] == 0)
			return 1;

		if (SaveGameName[i] != 32)
			return 0;
	}

	return 1;
}

/* FillInDefaultName__Fv (0x80012650) */
void FillInDefaultName(void) {
	if (!GSaveAction) {
		SaveGameName[0] = 'V';
		SaveGameName[1] = 'I';
		SaveGameName[2] = 'D';
		SaveGameName[3] = 'E';
		SaveGameName[4] = 'O';
		SaveGameName[5] = ' ';
		SaveGameName[6] = DAT_800d113c + 65;
		DAT_800d113c = (DAT_800d113c + 1) % 26;
	} else {
		SaveGameName[0] = 'C';
		SaveGameName[1] = 'A';
		SaveGameName[2] = 'R';
		SaveGameName[3] = 'E';
		SaveGameName[4] = 'E';
		SaveGameName[5] = 'R';
		SaveGameName[6] = ' ';
		SaveGameName[7] = DAT_800d1140 + 65;
		DAT_800d1140 = (DAT_800d1140 + 1) % 26;
	}
}

/* SaveReturnState__Fv (0x80012774) */
int SaveReturnState(void) {
	if (GSaveAction == 0) {
		if (InCompetition() && !Competition_IsOver())
			return 25;

		return 15;
	}

	return GReturnToState;
}

/* Front_MemCardUpdate__Fv (0x800127c4) */
/* From the demo twin + its MAIN.SYM locals (Activated, GoBack, AnyButton, Start;
   block locals UpDown, FileType, WriteError, save_name, WantedFileType/Vids/m/CardHead/p, pTmp).
   The memory-card front-end state machine; returns the next front-end screen (18 = stay).
   Levers: case bodies of the CardStatus sub-switches in demo source order (case 1 first); the
   string table pointers are fields of the front-end string struct GameText (lui of the struct
   first); the SaveFile/LoadFile name copy is an if/else of two calls; `if (!WriteError)` first; the
   video-header copy in case 1 reads NumFrames/Highlights through VH; "demoX.rec" is a 10-byte
   word-aligned block copy (strcpy of a literal), done as memcpy from the aligned extern. */
int Front_MemCardUpdate() {
	int Activated;
	int GoBack;
	int AnyButton;
	int Start;

	if (++TTime & 1)
		Card_CheckStatus(0, 0);

	Front_GetButtons(Activated, GoBack, AnyButton, Start);

	switch (MCState) {
		case 0:
			/* PC: no card to check (the saves are files, pc_saves.h) */
			MCState = 1;
			break;
			if (CheckingCardPause) {
				if (--CheckingCardPause == 0)
					MCState = 1;
			} else {
				CheckingCardPause = 10;
			}

			break;
		case 1:
			if (VideoToDisk) {
				MCState = 2;
				break;
			}

			switch (CardStatus) {
				case 1:
					/* PC: the saves of the type being saved (Card_GetNextTimeStamp looks through the
					   careers); there's always room */
					PC_SaveListDir(GSaveAction == 1 ? PC_SAVE_CAREER : PC_SAVE_REPLAY, SLOT_DIR,
					               (int32_t *)FileType);

					SaveFreeBlocks = 15;

					if (GSaveAction == 1) {
						SaveNeedBlocks = 1;

						if (SaveFreeBlocks < 1 && !GHasCareerLoaded) {
							MCState = 8;
							break;
						}

						if (GHasCareerLoaded) {
							{
								int i;

								for (i = 0; i < 10; i++)
									SaveGameName[i] = StoredCareerName[i];
							}

							MCState = 4;
							Delay = 4;
							Retries = 4;
						} else {
							MCState = 2;
							InitEnterName();
						}
					} else {
						SaveNeedBlocks = 3;

						if (GNumberOfPlayers == 2)
							SaveNeedBlocks = 6;

						if (SaveFreeBlocks < SaveNeedBlocks) {
							MCState = 8;
						} else {
							MCState = 2;
							InitEnterName();
						}
					}

					break;
				case -1:
					MCState = 9;
					break;
				case -2:
					MCState = 10;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pNo));
					break;
			}

			break;
		case 8:
			switch (CardStatus) {
				case -1:
					MCState = 9;
					break;
				case -2:
				case 2:
					MCState = 0;
					break;
			}

			if (AnyButton) {
				SFX_PlayX(SFX_SKID, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 2: {
			int UpDown = GetUpDown();

			if (UpDown == -1) {
				SizeUp = 16;
				SFX_PlayX(SFX_MENU_LETTER, 0x2000, 0);

				if (SaveGameName[CursorPos] == ' ' || SaveGameName[CursorPos] == 0) {
					SaveGameName[CursorPos] = 'A';
				} else {
					SaveGameName[CursorPos]++;

					if (SaveGameName[CursorPos] >= 91)
						SaveGameName[CursorPos] = ' ';
				}
			}

			if (UpDown == 1) {
				SizeUp = 16;
				SFX_PlayX(SFX_MENU_LETTER, 0x2000, 0);

				if (SaveGameName[CursorPos] == ' ' || SaveGameName[CursorPos] == 0) {
					SaveGameName[CursorPos] = 'Z';
				} else {
					SaveGameName[CursorPos]--;

					if (SaveGameName[CursorPos] < 65)
						SaveGameName[CursorPos] = ' ';
				}
			}

			if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered) {
				SizeUp = 16;
				SFX_PlayX(SFX_LAND, 0x2000, 0);

				Pad[0].MenuLeft.Triggered = 0;
				Pad[1].MenuLeft.Triggered = 0;
				CursorPos--;

				if (CursorPos < 0)
					CursorPos = 9;
			}

			if (Pad[0].MenuRight.Triggered || Pad[1].MenuRight.Triggered) {
				SizeUp = 16;
				SFX_PlayX(SFX_LAND, 0x2000, 0);

				Pad[0].MenuRight.Triggered = 0;
				Pad[1].MenuRight.Triggered = 0;
				CursorPos++;

				if (CursorPos >= 10)
					CursorPos = 0;
			}

			if (GSaveAction == 0)
				strcpy(s_800a42e8, SaveGameName);
			else
				strcpy(s_800a42dc, SaveGameName);

			if (GoBack) {
				SFX_PlayX(SFX_SKID, 0x2000, 0);
				return SaveReturnState();
			}

			if (CardStatus == -1 && !VideoToDisk)
				MCState = 9;

			if (Activated && !VideoToDisk) {
				SFX_PlayX(SFX_LAND, 0x2000, 0);

				{
					int FileType = GSaveAction == 0;

					if (NameWasBlank()) {
						do
							FillInDefaultName();

						while (Card_FileExists((EFileType)FileType));
					}

					if (Card_FileExists((EFileType)FileType) && !(GSaveAction == 1 && GHasCareerLoaded)) {
						MCState = 3;
						pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pNo));
						pYesNoMenu->CentreY();
					} else {
						MCState = 4;
						Delay = 4;
						Retries = 4;
					}
				}
			}

			if (Activated && VideoToDisk) {
				SFX_PlayX(SFX_LAND, 0x2000, 0);

				MCState = 5;
			}
		} break;
		case 3:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					SFX_PlayX(SFX_LAND, 0x2000, 0);

					MCState = 4;
					Delay = 4;
					Retries = 4;
				} else {
					SFX_PlayX(SFX_SKID, 0x2000, 0);

					MCState = 2;
					InitEnterName();
				}
			}

			if (GoBack) {
				SFX_PlayX(SFX_SKID, 0x2000, 0);

				MCState = 2;
				InitEnterName();
			}

			break;
		case 9:
			switch (CardStatus) {
				case -2:
				case 1:
				case 2:
					MCState = 0;
					break;
			}

			if (AnyButton) {
				SFX_PlayX(SFX_SKID, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 18:
			if (CardStatus == -1)
				MCState = 17;

			if (AnyButton) {
				SFX_PlayX(GoBack ? SFX_SHELL_BACK : SFX_SHELL_SELECT, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 10:
			pYesNoMenu->Update();

			switch (CardStatus) {
				case 1:
				case 2:
					MCState = 0;
					break;
				case -1:
					MCState = 9;
					break;
			}

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					SFX_PlayX(SFX_LAND, 0x2000, 0);

					MCState = 11;
					Delay = 4;
					Retries = 4;
				} else {
					SFX_PlayX(SFX_LAND, 0x2000, 0);
					return SaveReturnState();
				}
			}

			if (GoBack) {
				SFX_PlayX(SFX_SKID, 0x2000, 0);

				MCState = 0;
			}

			break;
		case 11:
			GSoftResetEnabled = 0;

			if (CardStatus == 2 || CardStatus == -1) {
				MCState = 12;
				Delay = 75;
				break;
			}

			if (--Delay < 0 && CardStatus) {
				if (Card_FormatCard(0, 0) == 1) {
					MCState = 13;
					Delay = 75;
				} else if (--Retries < 0) {
					MCState = 12;
					Delay = 75;
				} else {
					Delay = (4 - Retries) * 60;
				}
			}

			break;
		case 6:
		case 7:
			if (Activated) {
				SFX_PlayX(SFX_LAND, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 12:
		case 13:
			if (--Delay < 0) {
				if (CardStatus == -1)
					MCState = 9;
				else
					MCState = 0;
			}

			if (AnyButton) {
				SFX_PlayX(GoBack ? SFX_SKID : SFX_LAND, 0x2000, 0);

				MCState = 0;
			}

			break;
		case 4:
			GSoftResetEnabled = 0;

			if (CardStatus == -1) {
				MCState = 6;
				Delay = 75;
			}

			if (--Delay < 0 && CardStatus) {
				int WriteError;

				switch (GSaveAction) {
					case 0:
						SaveNeedBlocks = 3;

						if (GNumberOfPlayers == 2)
							SaveNeedBlocks = 6;

						WriteError = Card_Write(THPS_VIDEO_FILE, SaveNeedBlocks);
						break;
					case 1:
						SaveNeedBlocks = 1;
						WriteError = Card_Write(THPS_SAVE_FILE, SaveNeedBlocks);
						break;
				}

				if (!WriteError) {
					MCState = 7;

					if (GSaveAction == 1) {
						GHasCareerLoaded = 1;
						GLoadedCareerIsSaved = 1;

						{
							int i;

							for (i = 0; i < 10; i++)
								StoredCareerName[i] = SaveGameName[i];
						}
					}
				} else {
					MCState = 6;
					Delay = 75;
				}
			}

			break;
		case 5:
			GSoftResetEnabled = 0;

			if (SaveGameName[0] < 'A' || SaveGameName[0] > 'Z')
				SaveGameName[0] = 'A';

			if (SaveGameName[0] >= 'K') {
				MCState = 2;
				Activated = 0;
				GoBack = 0;
				break;
			}

			{
				char save_name[16];

				memcpy(save_name, "demoX.rec", 10);

				save_name[4] = SaveGameName[0];
				printf("Saving Replay %s", save_name);
				Card_WriteReplayToDisk(THPS_VIDEO_FILE, save_name);

				MCState = 7;
			}

			break;
		case 14:
			if (AnyButton)
				SFX_PlayX(SFX_LAND, 0x2000, 0);

			break;
		case 15:
			MCState = 16;
			break;
			if (CheckingCardPause) {
				if (--CheckingCardPause == 0)
					MCState = 16;
			} else {
				CheckingCardPause = 10;
			}

			break;
		case 16:
			switch (CardStatus) {
				case 1: {
					int WantedFileType;
					int Vids;
					int Offset;

					WantedFileType = 1;
					Offset = 26;

					if (GLoadAction == 2) {
						WantedFileType = 0;
						Offset = 28;
					}

					/* PC: only the saves of the type being loaded, so there can be 15 of each */
					PC_SaveListDir(WantedFileType, SLOT_DIR, (int32_t *)FileType);

					SaveFreeBlocks = 15;

					Vids = 0;

					{
						int i;

						for (i = 0; i < 15; i++)
							Vids += FileType[i] == WantedFileType;
					}

					if (Vids) {
						int m = 0;

						{
							int i;

							for (i = 0; i < 15; i++) {
								if (FileType[i] == WantedFileType) {
									SCardHead CardHead;
									char *p;

									Card_ReadFileHeader(SLOT_DIR[i], &CardHead);

									p = ASCIIName[m];

									{
										int j;

										for (j = 0; j < 10; j++) {
											if (!CardHead.Title[Offset + j * 2]) {
												*p++ = 0;
												break;
											}

											if ((uchar)CardHead.Title[Offset + j * 2] == 0x82)
												*p++ =
												    (uchar)CardHead.Title[Offset + j * 2 + 1] - 31;
											else
												*p++ = ' ';
										}
									}

									*p = 0;
									VertMenuItem[m] = ASCIIName[m];
									VertMenuItemIndex[m] = i;
									m++;
								}
							}
						}

						NumVertMenuItems = m;
						VertMenuTopIndex = 0;
						MCState = 20;
					} else {
						MCState = 19;
					}
				} break;
				case -1:
					MCState = 17;
					break;
				case -2:
					MCState = 18;
					break;
			}

			break;
		case 17:
			switch (CardStatus) {
				case -2:
				case 1:
				case 2:
					MCState = 15;
					break;
			}
		case 19:
			if (AnyButton) {
				SFX_PlayX(GoBack ? SFX_SHELL_BACK : SFX_SHELL_SELECT, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 20:
			VertMenuTopChanged = 0;

			if (Pad[0].MenuUp.Triggered || Pad[1].MenuUp.Triggered) {
				Pad[0].MenuUp.Triggered = 0;
				Pad[1].MenuUp.Triggered = 0;

				if (pVertMenu->getSelected() > 0) {
					pVertMenu->up();
				} else if (VertMenuTopIndex > 0) {
					VertMenuTopIndex--;
					VertMenuTopChanged = 1;
				}
			}

			if (Pad[0].MenuDown.Triggered || Pad[1].MenuDown.Triggered) {
				Pad[0].MenuDown.Triggered = 0;
				Pad[1].MenuDown.Triggered = 0;

				if (VertMenuTopIndex < NumVertMenuItems - 5) {
					VertMenuTopIndex++;
					VertMenuTopChanged = 1;
				} else if (pVertMenu->getSelected() < 4) {
					pVertMenu->down();
				}
			}

			if (GoBack || CardStatus == -1) {
				SFX_PlayX(SFX_SHELL_BACK, 0x2000, 0);
				return GReturnToState;
			}

			if (Activated) {
				Card_SetSelectedFile(
				    SLOT_DIR[VertMenuItemIndex[VertMenuTopIndex + pVertMenu->getSelected()]]);
				SFX_PlayX(SFX_SHELL_SELECT, 0x2000, 0);

				MCState = 21;
			}

			break;
		case 21:
			GSoftResetEnabled = 0;

			switch (GLoadAction) {
				case 0: {
					int Error;

					VblankUpdateBar = 1;
					Error = Card_Load(THPS_VIDEO_FILE, GVInfo1[0].pBlockStart, 0);
					VblankUpdateBar = 0;

					if (Error) {
						MCState = 22;
					} else {
						int i;
						SVideoHead *VH = (SVideoHead *)(GVInfo1[0].pBlockStart + 128);

						GVInfo1[0].NumFrames = VH->NumFrames;

						for (i = 0; i < 5; i++)
							GVInfo1[0].Highlights[i] = VH->Highlights[i];

						MCState = 23;
					}
				} break;
				case 1:
					if (Card_Load(THPS_VIDEO_HEADER, GVInfo1[0].pBlockStart, 0)) {
						MCState = 22;
					} else {
						{
							SVideoHead *VH = (SVideoHead *)(GVInfo1[0].pBlockStart + 128);

							GVideoRestart.NumSkaters = VH->NumSkaters;
							GVideoRestart.Skater = VH->Skater;
							GVideoRestart.Skater2 = VH->Skater2;
							GVideoRestart.Level = VH->Level;
							GVideoRestart.GGame = VH->GGame;

							{
								int i;

								GVInfo1[0].NumFrames = VH->NumFrames;

								for (i = 0; i < 5; i++)
									GVInfo1[0].Highlights[i] = VH->Highlights[i];
							}
						}

						MCState = 23;
					}

					break;
				case 2: {
					ulong *pTmp = (ulong *)Mem_NewX(3200, 1, 1);

					if (Card_Load(THPS_SAVE_FILE, pTmp, 0)) {
						MCState = 22;
					} else {
						GCareer = *(SCareer *)(pTmp + 160);
						Career_LoadExt(PC_SaveSelected());
						GHasCareerLoaded = 1;
						GLoadedCareerIsSaved = 0;
						Front_ExtractASCIINameFromSJIS((char *)pTmp + 32, StoredCareerName);

						MCState = 23;
					}

					Mem_Delete(pTmp);
				} break;
			}

			break;
		case 22:
			if (AnyButton) {
				SFX_PlayX(GoBack ? SFX_SHELL_BACK : SFX_SHELL_SELECT, 0x2000, 0);
				return 36;
			}

			break;
		case 23:
			return 35;
		case 27:
			break;
	}

	return 18;
}

/* Front_ClearGVideoRestartData__Fv (0x80013918) */
void Front_ClearGVideoRestartData(void) {
	GVideoRestart.NumSkaters = 0;
	GVideoRestart.Skater = -1;
	GVideoRestart.Skater2 = -1;
	GVideoRestart.Level = -1;
	GVideoRestart.GGame = -1;
}
