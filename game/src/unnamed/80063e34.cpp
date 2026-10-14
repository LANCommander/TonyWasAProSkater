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
		Pad[mWhich_pad].MenuLeft.Pressed = 0;
		Pad[mWhich_pad].MenuRight.Pressed = 0;
		Pad[mWhich_pad].MenuUp.Pressed = 0;
		Pad[mWhich_pad].MenuDown.Pressed = 0;
	} else {
		Pad[0].MenuLeft.Pressed = 0;
		Pad[0].MenuRight.Pressed = 0;
		Pad[0].MenuUp.Pressed = 0;
		Pad[0].MenuDown.Pressed = 0;
		Pad[1].MenuLeft.Pressed = 0;
		Pad[1].MenuRight.Pressed = 0;
		Pad[1].MenuUp.Pressed = 0;
		Pad[1].MenuDown.Pressed = 0;
	}
}
