/* FUN_800494dc (0x800494dc) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_screen.h"
#include "pc_draw.h"

extern int pBMR;

int DAT_800d1a38;

int Db_OTSize(void);
void CompactOTagR(P_TAG *ot, int n);

/* Draws the buffer being built now: its environment and display, the background picture
   (pBMR) when there is one, then its ordering table. */
void FUN_800494dc(void) {
	RECT r;

	PC_DrawSetEnv(&pDoubleBuffer->Draw);
	PC_ScreenShow(&pDoubleBuffer->Disp);

	if (DAT_800d1a38 && pBMR) {
		r.x = 0;
		r.y = (unsigned short)pDoubleBuffer->Top;
		r.w = 512;
		r.h = 240;
		PC_ScreenBlit(r.x, r.y, r.w, r.h, pBMR);
		DrawSync(0);
	} else {
		DAT_800d1a38 = 0;
	}

	CompactOTagR(&pDoubleBuffer->OT2D, Db_OTSize());

	if (DAT_800d1a38)
		DrawSync(0);

	PC_DrawOT(&(&pDoubleBuffer->OT2D)[Db_OTSize() - 1]);
}
