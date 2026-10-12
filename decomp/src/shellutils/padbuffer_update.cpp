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
			if (Pad[mWhich_pad].Left.Pressed)
				current = 1;

			if (Pad[mWhich_pad].Right.Pressed)
				current = 2;

			if (Pad[mWhich_pad].Up.Pressed)
				current = 3;

			if (Pad[mWhich_pad].Down.Pressed)
				current = 4;
		} else {
			if (Pad[0].Left.Pressed || Pad[1].Left.Pressed)
				current = 1;

			if (Pad[0].Right.Pressed || Pad[1].Right.Pressed)
				current = 2;

			if (Pad[0].Up.Pressed || Pad[1].Up.Pressed)
				current = 3;

			if (Pad[0].Down.Pressed || Pad[1].Down.Pressed)
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
			if (!Pad[mWhich_pad].Left.Pressed && !Pad[mWhich_pad].Right.Pressed &&
			    !Pad[mWhich_pad].Up.Pressed && !Pad[mWhich_pad].Down.Pressed)
				down = 0;
		} else {
			if (!Pad[0].Left.Pressed && !Pad[0].Right.Pressed &&
			    !Pad[0].Up.Pressed && !Pad[0].Down.Pressed &&
			    !Pad[1].Left.Pressed && !Pad[1].Right.Pressed &&
			    !Pad[1].Up.Pressed && !Pad[1].Down.Pressed)
				down = 0;
		}
	}
}
