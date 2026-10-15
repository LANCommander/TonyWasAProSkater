/* Panel_HoldTrick__FP6CBrucei (0x8006e1bc) */
#include "game/bruce.h"
#include "game/panel.h"

void Panel_HoldTrick(CBruce *p, int s) {
	p->mPanel.mTQ[p->mPanel.mTQStack - 1].point_value += s;
	p->mPanel.TrickMessageRefresh();
}
