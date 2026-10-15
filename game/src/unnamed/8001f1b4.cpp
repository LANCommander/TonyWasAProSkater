/* Mess_SetFontShadowOffset__FUc (0x8001f1b4) */
#include "game/font.h"

extern FontInfo Font_table[];

extern int Current_font;

void Mess_SetFontShadowOffset(uchar v) {
	Font_table[Current_font].mFont_shadow_offset = v;
}
