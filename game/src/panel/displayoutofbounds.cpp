/* DisplayOutOfBounds__FP6CBruceUsUs (0x8006d704) */
#include "game/bruce.h"
#include "game/front.h"

/* PANEL.CPP (demo twin; its pString local). Shows the out-of-bounds message (one of four by
   reason) as a text effect at the player's viewport offset until the effect completes. */
extern LocalText GameText;
extern int ViewportMode;

void Panel_GetViewportMessOffset(int, short *, short *);
void Mess_SetTextJustify(uchar);
void Mess_DrawPanelText(int, int, const char *, int, uint, int);

void DisplayOutOfBounds(CBruce *p, ushort a, ushort b) {
	short x, y;
	char *s0;

	if (p->mPanel.mOutOfBoundsMessage) {
		Panel_GetViewportMessOffset(ViewportMode * 2 + p->mPanel.mPlayerNum, &x, &y);

		switch (p->mPanel.mOutOfBoundsMessage) {
			case 1:
				s0 = GameText.pOutOfBounds1;
				break;
			case 2:
				s0 = GameText.pOutOfBounds2;
				break;
			case 3:
				s0 = GameText.pOutOfBounds3;
				break;
			default:
				s0 = GameText.pOutOfBounds4;
				break;
		}

		p->mPanel.mOutOfBoundsText.update();
		p->mPanel.mOutOfBoundsText.setScale(256);
		p->mPanel.mOutOfBoundsText.bind(1);
		Mess_SetTextJustify(0);
		Mess_DrawPanelText(a + x, b + y, s0, 0, 0, 256);

		if (p->mPanel.mOutOfBoundsText.handleComplete() == 0)
			p->mPanel.mOutOfBoundsMessage = 0;

		p->mPanel.mOutOfBoundsText.bind(0);
	}
}
