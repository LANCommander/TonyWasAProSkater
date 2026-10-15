/* Backgrnd_Off__FUs (0x8007813c) */
/* C++. Hides the background with the given index (clears its mFlags bit 0). */
#include "game/backgrnd.h"

extern CBody *BackgroundList;

void Backgrnd_Off(unsigned short id) {
	CBody *p;

	for (p = BackgroundList; p; p = (CBody *)p->mNextItem) {
		if (((CBackground *)p)->mIndex == id) {
			p->mFlags |= 1;
			return;
		}
	}
}
