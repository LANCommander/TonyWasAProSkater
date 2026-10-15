/* Backgrnd_On__FUs (0x800780f0) */
#include "game/backgrnd.h"

CBody *BackgroundList;

void Backgrnd_On(ushort id) {
	CBody *p;

	for (p = BackgroundList; p; p = (CBody *)p->mNextItem) {
		if (((CBackground *)p)->mIndex == id) {
			p->mFlags &= ~1;
			return;
		}
	}
}
