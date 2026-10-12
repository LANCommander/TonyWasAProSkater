/* __5CMenuiiUciii (0x80016534) */
/* C++ (FRONT.CPP). Sets the menu's position, justification and line separation, gives every line
   the default highlight (orange) and normal (grey) colours and scales, and selects line 0. */
#include "game/front.h"

CMenu::CMenu(int x, int y, uchar justification, int hiScale, int lowScale, int lineSep) {
	mX = x;
	mY = y;
	mJustification = justification;
	mLineSep = lineSep;

	for (int i = 0; i < 15; i++) {
		mEntry[i].HiR = 182;
		mEntry[i].HiG = 132;
		mEntry[i].HiB = 36;
		mEntry[i].LowR = 122;
		mEntry[i].LowG = 122;
		mEntry[i].LowB = 122;
		mEntry[i].FadeUpCounter = 128;
		mEntry[i].HiScale = hiScale;
		mEntry[i].LowScale = lowScale;
	}

	SetLine(0);
}
