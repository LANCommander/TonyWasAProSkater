/* CentreY__5CMenu (0x80016680) */
/* C++ (FRONT.CPP). Centres the visible lines (line separation plus each line's clearance) on the
   240-line screen. */
#include "game/front.h"

void CMenu::CentreY() {
	int count = 0;
	int height = 0;

	for (int i = 0; i < mNumLines; i++) {
		if (mEntry[i].Visible) {
			count++;
			height += mEntry[i].Clearance;
		}
	}

	mY = (240 - ((count - 1) * mLineSep + height)) / 2;
}
