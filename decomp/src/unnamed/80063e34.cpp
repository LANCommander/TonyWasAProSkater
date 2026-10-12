/* __9PadBufferi (0x80063e34) */
/* C++ (SHELLUTILS.CPP). An empty d-pad queue for controller pad (both when >= 2; forced to pad 0
   when DAT_800d1d9c is set); clears the d-pad pressed flags of the pad(s) it reads. */
#include "game/shellutils.h"
#include "game/pad.h"

extern SControl Pad[];

int DAT_800d1d9c;

PadBuffer::PadBuffer(int pad) {
	entries = 0;
	down = 0;
	last = 0;
	first = 0;
	mWhich_pad = pad;

	if (DAT_800d1d9c)
		mWhich_pad = 0;

	if (mWhich_pad < 2) {
		Pad[mWhich_pad].Left.Pressed = 0;
		Pad[mWhich_pad].Right.Pressed = 0;
		Pad[mWhich_pad].Up.Pressed = 0;
		Pad[mWhich_pad].Down.Pressed = 0;
	} else {
		Pad[0].Left.Pressed = 0;
		Pad[0].Right.Pressed = 0;
		Pad[0].Up.Pressed = 0;
		Pad[0].Down.Pressed = 0;
		Pad[1].Left.Pressed = 0;
		Pad[1].Right.Pressed = 0;
		Pad[1].Up.Pressed = 0;
		Pad[1].Down.Pressed = 0;
	}
}
