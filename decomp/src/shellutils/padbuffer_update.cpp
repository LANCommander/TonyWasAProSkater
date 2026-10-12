/* update__9PadBuffer (0x80063f58) */
/* PadBuffer: a small queue of d-pad presses for the front-end menus. update() samples the
   Left/Right/Up/Down "Pressed" flags of one controller (or of both when mWhich_pad >= 2), queues one
   press (1..4) per push and waits for every direction to be released before queueing again. */
#include "game/shellutils.h"
#include "game/pad.h"

extern SControl Pad[];

void PadBuffer::update() {
	if (!down) {
		current = 0;

		if (mWhich_pad < 2) {
			if (Pad[mWhich_pad].MenuLeft.Pressed)
				current = 1;

			if (Pad[mWhich_pad].MenuRight.Pressed)
				current = 2;

			if (Pad[mWhich_pad].MenuUp.Pressed)
				current = 3;

			if (Pad[mWhich_pad].MenuDown.Pressed)
				current = 4;
		} else {
			if (Pad[0].MenuLeft.Pressed || Pad[1].MenuLeft.Pressed)
				current = 1;

			if (Pad[0].MenuRight.Pressed || Pad[1].MenuRight.Pressed)
				current = 2;

			if (Pad[0].MenuUp.Pressed || Pad[1].MenuUp.Pressed)
				current = 3;

			if (Pad[0].MenuDown.Pressed || Pad[1].MenuDown.Pressed)
				current = 4;
		}

		if (current && entries < 8) {
			buffer[last++] = current;

			if (last >= 8)
				last = 0;

			down = 1;
			entries++;
		}
	} else {
		if (mWhich_pad < 2) {
			if (!Pad[mWhich_pad].MenuLeft.Pressed && !Pad[mWhich_pad].MenuRight.Pressed &&
			    !Pad[mWhich_pad].MenuUp.Pressed && !Pad[mWhich_pad].MenuDown.Pressed)
				down = 0;
		} else {
			if (!Pad[0].MenuLeft.Pressed && !Pad[0].MenuRight.Pressed &&
			    !Pad[0].MenuUp.Pressed && !Pad[0].MenuDown.Pressed &&
			    !Pad[1].MenuLeft.Pressed && !Pad[1].MenuRight.Pressed &&
			    !Pad[1].MenuUp.Pressed && !Pad[1].MenuDown.Pressed)
				down = 0;
		}
	}
}
