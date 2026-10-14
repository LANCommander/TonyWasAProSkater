/* SetLine__5CMenuUc (0x80016708) */
/* C++ (FRONT.CPP). Demo twin (same lines). Selects menu line Line: it scales up, the rest down. */
#include "game/front.h"

void CMenu::SetLine(uchar Line) {
	mLine = Line;

	for (int i = 0; i < 15; i++) {
		mEntry[i].TargetScale = (i == mLine) ? mEntry[i].HiScale : mEntry[i].LowScale;
		mEntry[i].CurrentScale = mEntry[i].TargetScale;
	}
}
