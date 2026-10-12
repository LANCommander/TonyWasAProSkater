/* Front_MemCardHiResDisplay__Fv (0x8001206c) */
/* C++ (RFRONT.CPP), from the demo twin + its MAIN.SYM locals (i, XMid, block `load`, block `up`).
   Hi-res memory-card screen: the save/load progress screen, the background state machine, then
   one case per card state (case 20 = the save-file VerticalMenu with scroll arrows).
   Levers: the arrow text is `if (up && down) ... else sprintf(.., up ? a : b)` with `down` a
   separate block local (keeps up in a1 and XMid in s1); FUN_8008c468 returns int; the
   VerticalMenu ?: count is `n < 6 ? n : 5`; sizeof(VerticalMenu) = 820. */
#include "game/shellutils.h"
#include "game/front.h"

extern "C" {
	extern LocalText GameText;
	extern char *GameText_180[];
	extern char *VertMenuItem[];
	extern char s_loading_video_file[];
	extern char s_loading_saved_game[];
	extern char s_Video_loaded[];
	extern char msg[];
	extern int TTime;
}

VerticalMenu *pVertMenu;
int NumVertMenuItems, VertMenuTopIndex, VertMenuTopChanged, MCState, OldMCState, MCResolution,
    GLoadAction;

extern "C" {
	void FUN_80011214(void);
	void FUN_80011570(void);
	void Mess_SetTextJustify__FUc(int);
	void FUN_80061ea0(void);
	void FUN_8006b240(void);
	void FUN_8006b26c(void);
	void FUN_8008b900(char *, char *, char *);
	int FUN_8008c468(int);
}

void Front_DirtyRectText(int, int, const char *);
void Front_PrintCentredPrompt(const char *);
void Front_PrintGoBackSelect();
void Front_RestoreBackground();
void Mess_SetScale(int);
void RGBRed();

void Front_MemCardHiResDisplay() {
	int i;
	int XMid;

	Mess_SetScale(200);

	XMid = MCResolution >> 1;

	if (MCState == 21 && GLoadAction == 2) {
		{
			char *load;

			for (i = 0; i < 2; i++) {
				FUN_8006b240();
				FUN_80061ea0();

				if (pVertMenu)
					pVertMenu->erase();

				FUN_8006b26c();
			}

			load = s_loading_saved_game;
			FUN_8008c468(0);

			for (i = 0; i < 4; i++) {
				FUN_8006b240();
				FUN_80061ea0();
				FUN_80011214();
				Front_DirtyRectText(XMid, 240, load);
				RGBRed();
				FUN_8006b26c();
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

			FUN_80011570();
		set:
			OldMCState = MCState;
		}
	}

	Mess_SetScale(200);
	Mess_SetTextJustify__FUc(0);

	switch (MCState) {
		case 15:
		case 16:
			FUN_80011214();
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
				FUN_8008b900(msg, GameText.pHiResScroll, GameText.pHiResUpOrDown);
				Front_DirtyRectText(XMid, 380, msg);
			}

			for (i = 0; i < 5; i++) {
				if (i != pVertMenu->getSelected() || VertMenuTopChanged)
					pVertMenu->setString(i, VertMenuItem[VertMenuTopIndex + i]);
			}

			if (NumVertMenuItems > 5 && ((TTime / 20) & 3)) {
				FUN_80011214();

				{
					int up = VertMenuTopIndex > 0;
					int down = VertMenuTopIndex < NumVertMenuItems - 5;

					if (up && down)
						FUN_8008b900(msg, GameText.pHiResScroll, GameText.pHiResUpOrDown);
					else
						FUN_8008b900(msg, GameText.pHiResScroll,
						             up ? GameText.pHiResUp : GameText.pHiResDown);
				}

				Front_DirtyRectText(XMid, 380, msg);
			}

			pVertMenu->draw();

			if (MCResolution == 512)
				Front_PrintGoBackSelect();

			break;
		case 21:
			FUN_80011214();
			Front_DirtyRectText(XMid, 220, s_loading_video_file);
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
			Front_DirtyRectText(XMid, 220, s_Video_loaded);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText_180[0]);

			break;
		case 27:
			break;
	}
}
