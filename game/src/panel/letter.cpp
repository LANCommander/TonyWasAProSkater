/* Panel_Letter__Fi (0x8006e24c) */
/* C++: the skater's S-K-A-T-E letters are CBruce::mPanel.SKATE (game/bruce.h, game/panel.h). */
#include "game/bruce.h"

extern int SKATE_Display;

extern CBruce *MechList;

void Panel_Tape(int);

int Panel_Letter(int n) {
	SKATE_Display = 40;

	if ((unsigned)n < 5)
		MechList->mPanel.SKATE[n] = 1;

	if (MechList->mPanel.SKATE[0] == 1 && MechList->mPanel.SKATE[1] == 1 &&
	    MechList->mPanel.SKATE[2] == 1 && MechList->mPanel.SKATE[3] == 1 &&
	    MechList->mPanel.SKATE[4] == 1) {
		Panel_Tape(2);
		return 1;
	}

	return 0;
}
