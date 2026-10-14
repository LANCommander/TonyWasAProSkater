/* DisplayTapesSkate__Fv (0x8006c7e8) */
#include "game/bruce.h"

extern CBruce *MechList;
extern uint Xblanks;

int TAPES_Display;
int TAPES_Pos;
int SKATE_Display;
int SKATE_Pos;

void Mess_SetRGB(uchar, uchar, uchar, int);
void Mess_SetTextJustify(uchar);
void Mess_SetScale(int);
void Mess_DrawPanelText(int, int, const char *, int, uint, int);

void DisplayTapesSkate(void) {
	CBruce *pSkater = MechList;

	if (!pSkater->mNextItem) {
		if (SKATE_Display) {
			if (SKATE_Pos >= 483 && SKATE_Display > 10)
				SKATE_Pos -= 4;

			if (SKATE_Display < 10)
				SKATE_Pos = 522 - SKATE_Display * 4;

			SKATE_Display--;
			Mess_SetRGB(((Xblanks % 20) * 12) & 252, 100, 100, 0);
			Mess_SetTextJustify(1);
			Mess_SetScale(255);

			if (pSkater->mPanel.SKATE[0])
				Mess_DrawPanelText(SKATE_Pos, 80, "S", 0, 0, 256);

			if (pSkater->mPanel.SKATE[1])
				Mess_DrawPanelText(SKATE_Pos, 105, "K", 0, 0, 256);

			if (pSkater->mPanel.SKATE[2])
				Mess_DrawPanelText(SKATE_Pos, 130, "A", 0, 0, 256);

			if (pSkater->mPanel.SKATE[3])
				Mess_DrawPanelText(SKATE_Pos, 155, "T", 0, 0, 256);

			if (pSkater->mPanel.SKATE[4])
				Mess_DrawPanelText(SKATE_Pos, 180, "E", 0, 0, 256);
		}

		if (TAPES_Display) {
			if (TAPES_Pos >= 482 && TAPES_Display > 10)
				TAPES_Pos -= 4;

			if (TAPES_Display < 11)
				TAPES_Pos = 511 - TAPES_Display * 4;

			TAPES_Display--;
		}
	}
}
