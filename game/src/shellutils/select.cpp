/* select__12VerticalMenui (0x80065120) */
/* C++ (SHELLUTILS.CPP). Selects item i unless it's greyed out, restarting the selection animation
   (tick 62 if the triangles are still at rest, else 64). */
#include "game/shellutils.h"

void VerticalMenu::select(int i) {
	if (!list[i].grayed) {
		old_selected = selected;
		selected = i;

		if (tick > 60)
			tick = 62;
		else
			tick = 64;
	}
}
