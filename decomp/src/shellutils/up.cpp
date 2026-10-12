/* up__12VerticalMenu (0x80065178) */
/* C++ (SHELLUTILS.CPP). Selects the previous non-greyed item (wrapping); while the selection is
   still animating, swallows the button presses instead. */
#include "game/shellutils.h"
#include "game/pad.h"

extern SControl Pad[];

void VerticalMenu::up() {
	if (tick > 60) {
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);
	} else {
		int i = selected - 1;

		if (i < 0)
			i = num_items - 1;

		while (i != selected && list[i].grayed) {
			i--;

			if (i < 0)
				i = num_items - 1;
		}

		select(i);
	}
}
