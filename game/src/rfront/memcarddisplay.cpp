/* Front_MemCardDisplay__Fv (0x80011820) */
#include "game/front.h"
#include "game/mess.h"
#include "game/psyq.h"
#include "game/rfront.h"

/* Memory-card front-end screen (RFRONT.CPP). The save-slot list loop follows the demo twin:
   x is XMid - 150 + i * 30 (no separate column variable), and the slot letter goes into a copy of a
   3-byte template in buf. */
extern LocalText GameText;
extern char SaveGameName[];

typedef struct {
	char a, b, c;
} T3;

extern char DAT_800d1138[];

extern CMenu *pYesNoMenu;
extern int TTime;

extern unsigned MCState;
extern int MCResolution;
extern int CursorPos, SizeUp, SaveNeedBlocks, SaveFreeBlocks;
extern int GLoadAction;

void Front_PrintCentredPrompt(const char *);
void Front_PrintGoBackSelect();
void RGBRed();

void Front_MemCardDisplay() {
	int s2, s0, s1, t;
	char buf[24];
	unsigned char c;
	CMenu *o;
	char *a;

	s2 = MCResolution >> 1;
	Mess_SetScale(256);
	Mess_SetTextJustify(0);

	switch (MCState) {
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
					*(T3 *)buf = *(T3 *)DAT_800d1138;
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
