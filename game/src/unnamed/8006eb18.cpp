/* FUN_8006eb18 (0x8006eb18) */
#include "game/bruce.h"

struct SMessageProg;

extern int ViewportMode;
extern CBruce *GSkater;
extern char *GameText[];

extern SMessageProg Messprog_Goal;

void Panel_GetViewportMessOffset(int a, short *b, short *c);
void Mess_Message(const char *, SMessageProg *, bool, short, short);

extern "C" void FUN_8006eb18(CPanelData *p) {
	short x, y;

	Panel_GetViewportMessOffset(ViewportMode * 2 + p->mpBruce->mPanel.mPlayerNum, &x, &y);

	if (p->mpBruce == GSkater)
		Mess_Message(GameText[78], &Messprog_Goal, 0, x, y);
	else
		Mess_Message(GameText[79], &Messprog_Goal, 0, x, y);
}
