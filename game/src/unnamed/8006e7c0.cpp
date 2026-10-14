/* TrickMessage__10CPanelData (0x8006e7c0) */
/* C++ (PANEL.CPP). Starts showing the trick message (and refreshes its text) while tricks are
   queued; otherwise turns it off. */
#include "game/panel.h"

void CPanelData::TrickMessage() {
	if (mTQStack) {
		mTrickState = 1;
		mTrickTimer = 0;
		TrickMessageRefresh();
	} else
		mTrickState = 0;
}
