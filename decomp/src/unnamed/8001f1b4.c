/* Mess_SetFontShadowOffset__FUc (0x8001f1b4) */
#include "names.h"

extern char Font_table[];

int Current_font;

void Mess_SetFontShadowOffset(char v) {
	Font_table[Current_font * 36 + 24] = v;
}
