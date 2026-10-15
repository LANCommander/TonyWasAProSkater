/* FUN_8005dcc4 (0x8005dcc4) */
#include "pc_screen.h"
/* Draws the countdown bar started by FUN_8005dc58: elapsed vblanks since the start, capped at the
   total, as a 400x8 bar (dark red, filling) at one of three screen positions; a full blue bar once
   time is up or when no total is set. ClearImage is ClearImage (it returns a value). */
typedef struct {
	short x, y, w, h;
} R;

extern unsigned short DAT_800bf0f0[];
extern unsigned short DAT_800bf0fc[];
extern unsigned short DAT_800bf0ec[];
extern unsigned short DAT_800bf0f8[];
extern unsigned short DAT_800bf0f4[];
extern unsigned short DAT_800bf100[];

extern int BackOut;
extern int DAT_800d1ce4;
extern int DAT_800d1cd8;
extern unsigned DAT_800d1ce0;
extern unsigned DAT_800d1cdc;
extern int DAT_800d1cf8;

extern unsigned Vblanks;

int ClearImage(R *r, int red, int green, int blue);

void FUN_8005dcc4(void) {
	R r;

	if (BackOut)
		return;

	if (!DAT_800d1ce4)
		return;

	DAT_800d1cdc = Vblanks - DAT_800d1cd8;

	if (DAT_800d1ce0 < DAT_800d1cdc)
		DAT_800d1cdc = DAT_800d1ce0;

	if (DAT_800d1cf8 == 4) {
		r.x = DAT_800bf0f0[0];
		r.y = DAT_800bf0fc[0];
	} else if (DAT_800d1cf8 == 0) {
		r.x = DAT_800bf0ec[0];
		r.y = DAT_800bf0f8[0];
	} else {
		r.x = DAT_800bf0f4[0];
		r.y = DAT_800bf100[0];
	}

	r.w = 0;
	r.h = 8;

	if (DAT_800d1ce0 == 0 || DAT_800d1ce0 < DAT_800d1cdc) {
		r.w = 400;
		PC_ScreenFill(r.x, r.y, r.w, r.h, 0xff0000);
	} else {
		r.w = (DAT_800d1cdc * 400) / DAT_800d1ce0;
		PC_ScreenFill(r.x, r.y, r.w, r.h, 0x000096);
	}
}
