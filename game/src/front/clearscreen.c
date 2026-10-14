/* Front_ClearScreen__Fv (0x80017148) */
#include "pc_screen.h"
typedef struct {
	short x, y, w, h;
} RECT;

extern int GameFade;
extern RECT DAT_800d1444[];

int ClearImage(RECT *, int, int, int);
int DrawSync(int);

void Front_ClearScreen(void) {
	RECT r;

	GameFade = 0;
	r = DAT_800d1444[0];
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);

	r.y = 511;
	r.h = 1;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	DrawSync(0);
}
