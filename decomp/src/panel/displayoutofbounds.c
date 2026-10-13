/* DisplayOutOfBounds__FP6CBruceUsUs (0x8006d704) */
#include "names.h"

/* PANEL.CPP, as C (demo twin; its pString local). Shows the out-of-bounds message (one of four by
   reason) as a text effect at the player's viewport offset until the effect completes. */
typedef struct {
	char p0[0x92c];
	unsigned short f92c;
	char p1[0xad0 - 0x92e];
	int fad0;
	int fad4[1];
} B;

extern int GameText_168[], GameText_16c[], GameText_170[], GameText_174[];
extern int ViewportMode;

void Panel_GetViewportMessOffset(int, short *, short *);
void FallingTextEffect_update(int *);
void FallingTextEffect_bind(int *, int);
void FallingTextEffect_setScale(int *, int);
int FallingTextEffect_handleComplete(int *);
void Mess_SetTextJustify(int);
void Mess_DrawPanelText(int, int, char *, int, int, int);

void DisplayOutOfBounds(B *p, int a, int b) {
	short x, y;
	char *s0;

	a &= 0xffff;
	b &= 0xffff;

	if (p->fad0) {
		Panel_GetViewportMessOffset(ViewportMode * 2 + p->f92c, &x, &y);

		switch (p->fad0) {
			case 1:
				s0 = (char *)GameText_168[0];
				break;
			case 2:
				s0 = (char *)GameText_16c[0];
				break;
			case 3:
				s0 = (char *)GameText_170[0];
				break;
			default:
				s0 = (char *)GameText_174[0];
				break;
		}

		FallingTextEffect_update(p->fad4);
		FallingTextEffect_setScale(p->fad4, 256);
		FallingTextEffect_bind(p->fad4, 1);
		Mess_SetTextJustify(0);
		Mess_DrawPanelText(a + x, b + y, s0, 0, 0, 256);

		if (FallingTextEffect_handleComplete(p->fad4) == 0)
			p->fad0 = 0;

		FallingTextEffect_bind(p->fad4, 0);
	}
}
