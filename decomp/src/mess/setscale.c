/* Mess_SetScale__Fi (0x8001d25c) */
#include "names.h"

unsigned char NewFont;
short Scale;

extern void Mess_SetFontScale(unsigned);

void Mess_SetScale(int s) {
	if (NewFont)
		Mess_SetFontScale(s);
	else
		Scale = s;
}
