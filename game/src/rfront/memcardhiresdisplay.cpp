/* Front_MemCardHiResDisplay__Fv (0x8001206c) */
/* C++ (RFRONT.CPP), from the demo twin + its MAIN.SYM locals (i, XMid, block `load`, block `up`).
   Hi-res memory-card screen: the save/load progress screen, the background state machine, then
   one case per card state (case 20 = the save-file VerticalMenu with scroll arrows).
   Levers: the arrow text is `if (up && down) ... else sprintf(.., up ? a : b)` with `down` a
   separate block local (keeps up in a1 and XMid in s1); DrawSync returns int; the
   VerticalMenu ?: count is `n < 6 ? n : 5`; sizeof(VerticalMenu) = 820. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#include "game/front.h"
#include "game/mess.h"
#include "game/psyq.h"
#include "game/hires.h"
#include "game/rfront.h"

extern "C" {
	extern LocalText GameText;
	extern char *VertMenuItem[];
	extern char msg[];
	extern int TTime;
}

extern VerticalMenu *pVertMenu;
extern int NumVertMenuItems, VertMenuTopIndex, VertMenuTopChanged, MCState, OldMCState, MCResolution, GLoadAction;

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
				Front_PrintCentredPrompt(((char **)&GameText)[96]);

			break;
		case 27:
			break;
	}
}
