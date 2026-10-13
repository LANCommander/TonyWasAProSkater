/* HiRes_BlankScreen__Fv (0x8006af64) */
#include "names.h"
/* Blanks the display: clears the screen rectangle and the bottom VRAM line, then re-puts the
   draw environment with dithering-on-display and background clear turned off. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

extern RECT DAT_800d1e20[];
extern DRAWENV DoubleBuffer;

void HiRes_BlankScreen(void) {
	RECT r;

	SetDispMask(0);

	r = DAT_800d1e20[0];
	ClearImage(&r, 0, 0, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	ClearImage(&r, 0, 0, 0);
	DrawSync(0);

	DoubleBuffer.dfe = 0;
	DoubleBuffer.isbg = 0;
	PutDrawEnv(&DoubleBuffer);
	SetDispMask(1);
}
