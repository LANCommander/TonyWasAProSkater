/* Panel_PlayerInit__FUsP6CBruce (0x8006e144) */
#include "game/bruce.h"

extern CBruce *MechList;

void Panel_PlayerInit(unsigned short n, CBruce *b) {
	if (b == MechList)
		b->mPanel.mPlayerNum = 0;
	else
		b->mPanel.mPlayerNum = 1;
}
