/* FUN_8006e7fc (0x8006e7fc) */
/* C++ (PANEL.CPP): CPanelData's trick-status setter. */
#include "game/panel.h"

extern "C" void FUN_8006e7fc(CPanelData *p, int v) {
	p->mTrickStatus = v;
}
