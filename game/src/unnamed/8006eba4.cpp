/* OutOfBounds__10CPanelData (0x8006eba4) */
/* C++ (PANEL.CPP). Picks one of the four out-of-bounds messages and restarts its falling text. */
#include "game/panel.h"

int Redbook_Rnd(int);

void CPanelData::OutOfBounds() {
	mOutOfBoundsMessage = Redbook_Rnd(4) + 1;
	mOutOfBoundsText.init();
}
