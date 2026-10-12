/* down__12VerticalMenu (0x80065230) */
/* C++ (SHELLUTILS.CPP). Selects the next non-greyed item (wrapping); while the selection is still
   animating, swallows the button presses instead. */
#include "game/shellutils.h"
#include "game/pad.h"

extern SControl Pad[];

void VerticalMenu::down() {
	if (tick > 60) {
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);
	} else {
		int i = selected + 1;

		if (i >= num_items)
			i = 0;

		while (i != selected && list[i].grayed) {
			i++;

			if (i >= num_items)
				i = 0;
		}

		select(i);
	}
}
