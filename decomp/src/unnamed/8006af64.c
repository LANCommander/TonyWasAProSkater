/* HiRes_BlankScreen__Fv (0x8006af64) */
#include "names.h"
/* Blanks the display: clears the screen rectangle and the bottom VRAM line, then re-puts the
   draw environment with dithering-on-display and background clear turned off. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#ifdef PC_PORT
#include "pc_screen.h"
#include "pc_draw.h"
#endif

extern RECT DAT_800d1e20[];
extern DRAWENV DoubleBuffer;

void HiRes_BlankScreen(void) {
	RECT r;

#ifdef PC_PORT
	PC_ScreenEnable(0);
#else
	SetDispMask(0);
#endif

	r = DAT_800d1e20[0];
#ifdef PC_PORT
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
#else
	ClearImage(&r, 0, 0, 0);
#endif
	DrawSync(0);

	r.y = 511;
	r.h = 1;
#ifdef PC_PORT
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
#else
	ClearImage(&r, 0, 0, 0);
#endif
	DrawSync(0);

	DoubleBuffer.dfe = 0;
	DoubleBuffer.isbg = 0;
#ifdef PC_PORT
	PC_DrawSetEnv(&DoubleBuffer);
	PC_ScreenEnable(1);
#else
	PutDrawEnv(&DoubleBuffer);
	SetDispMask(1);
#endif
}
