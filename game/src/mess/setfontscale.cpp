/* Mess_SetFontScale__FUi (0x8001f1d8) */


extern unsigned char NewFont;
extern int Current_font;

#include "game/font.h"

extern FontInfo Font_table[];

void Mess_SetScale(int);

void Mess_SetFontScale(uint s) {
	if (!NewFont)
		Mess_SetScale(s);
	else
		Font_table[Current_font].mFont_scale = s;
}
