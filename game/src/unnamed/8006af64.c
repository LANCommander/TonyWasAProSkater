/* HiRes_BlankScreen__Fv (0x8006af64) */
#include "names.h"
/* Blanks the display: clears the screen rectangle and the bottom VRAM line, then re-puts the
   draw environment with dithering-on-display and background clear turned off. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "pc_screen.h"
#include "pc_draw.h"

extern RECT DAT_800d1e20[];
#include "game/dbuffer.h"

void HiRes_BlankScreen(void) {
	RECT r;

	PC_ScreenEnable(0);

	r = DAT_800d1e20[0];
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	DoubleBuffer[0].Draw.dfe = 0;
	DoubleBuffer[0].Draw.isbg = 0;
	PC_DrawSetEnv(&DoubleBuffer[0].Draw);
	PC_ScreenEnable(1);
}
