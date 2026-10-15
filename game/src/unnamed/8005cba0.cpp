/* LoadingScreen_Main__Fv (0x8005cba0) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}
#include "pc_frame.h"
#include "pc_screen.h"
#include "pc_draw.h"

#include "game/career.h"
#include "game/pad.h"

extern SLevel Levels[];
extern SControl Pad[];

extern int BackOut, DAT_800d1cf4;
extern unsigned DAT_800d1cf8;

extern "C" DRAWENV *FUN_80092e70(DRAWENV *, int, int, int, int);
extern "C" void FUN_8005cd14(void);
extern "C" void FUN_8005d00c(void);
extern "C" void FUN_8005d28c(void);

void LoadingScreen_Main() {
	DISPENV de;
	DRAWENV dr;
	RECT r;

	BackOut = 0;
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	FUN_80092e70(&dr, 0, 0, 512, 240);
	SetDefDispEnv(&de, 0, 0, 512, 240);

	r.x = 0;
	r.y = 0;
	r.w = 512;
	r.h = 240;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	PC_FrameWait(1);
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);

	dr.dfe = 0;
	dr.isbg = 0;
	dr.r0 = 255;
	dr.g0 = 0;
	dr.b0 = 0;
	PC_DrawSetEnv(&dr);
	PC_ScreenShow(&de);

	if (DAT_800d1cf8 < 6) {
		switch (DAT_800d1cf8) {
			case 0:
				if (Levels[DAT_800d1cf4].isCompetition != 0) {
					FUN_8005cd14();
					return;
				}

				FUN_8005d00c();
				return;
			case 1:
			case 3:
			case 5:
				FUN_8005cd14();
				return;
			case 2:
				FUN_8005d28c();
				break;
		}
	}
}
