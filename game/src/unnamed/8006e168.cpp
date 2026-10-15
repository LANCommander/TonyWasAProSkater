/* Panel_TakeOff__FP6CBrucei (0x8006e168) */
/* C++. The skater takes off: sets the panel's trick status, clears old_180spins, starts the replay
   highlight. */
#include "game/bruce.h"

extern "C" void FUN_8006e7fc(CPanelData *p, int v);
void Video_HighlightStart(CBruce *p);

void Panel_TakeOff(CBruce *p, int a) {
	FUN_8006e7fc(&p->mPanel, a);

	p->mPanel.old_180spins = 0;
	Video_HighlightStart(p);
}
