/* Mess_GetFontScale__Fv (0x8001f22c) */

extern unsigned char NewFont;
extern int Current_font;

#include "game/font.h"

extern FontInfo Font_table[];

int Mess_GetFontScale() {
	if (!NewFont)
		return 0;

	return Font_table[Current_font].mFont_scale;
}
