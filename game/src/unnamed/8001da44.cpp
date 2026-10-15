/* Mess_TextWidth__FPCc (0x8001da44) */
/* C++. A string's width in the current font, scaled (the old fixed font: FUN_8001d340): each
   character's width (real_w; a space: the font's space width) plus the letter spacing. */
#include "game/font.h"

extern FontInfo Font_table[];

extern int Current_font;
extern uchar NewFont;

extern "C" int FUN_8001d340(const uchar *);
int GetCharIndex(char);

int Mess_TextWidth(const char *s0) {
	int n, idx;
	const uchar *s;

	if (NewFont == 0)
		return FUN_8001d340((const uchar *)s0);

	s = (const uchar *)s0;
	n = 0;

	if (*s) {
		do {
			idx = GetCharIndex(*s);

			if ((unsigned)(idx - 250) >= 5) {
				if (idx == 255) {
					n += ((Font_table[Current_font].mSpace_width + Font_table[Current_font].mChar_sep) *
					      Font_table[Current_font].mFont_scale) >>
					     8;
				} else {
					n += ((Font_table[Current_font].mChar_table[idx].real_w +
					       Font_table[Current_font].mChar_sep) *
					      Font_table[Current_font].mFont_scale) >>
					     8;
				}
			}

			s++;
		} while (*s);
	}

	return n;
}
