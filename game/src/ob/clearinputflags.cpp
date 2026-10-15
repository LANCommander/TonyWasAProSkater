/* Ob_ClearInputFlags__FP5CBody (0x80044484) */
#include "game/ob.h"

void Ob_ClearInputFlags(CBody *b) {
	CBody *n;

	if (b) {
		n = (CBody *)b->mNextItem;

		while (1) {
			b->mInputFlags &= 1;
			b = n;

			if (!b)
				break;

			n = (CBody *)b->mNextItem;
		}
	}
}
