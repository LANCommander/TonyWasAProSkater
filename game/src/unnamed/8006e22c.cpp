/* Panel_Bail__FP6CBruce (0x8006e22c) */
#include "game/bruce.h"

extern "C" void FUN_8006ea4c(CPanelData *p);

void Panel_Bail(CBruce *p) {
	FUN_8006ea4c(&p->mPanel);
}
