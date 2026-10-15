/* Mess_TextHeightBelowBaseline__FPCc (0x8001db80) */

#include "game/font.h"

extern FontInfo Font_table[];

uchar NewFont;
int Current_font;

extern "C" int FUN_8001d548(const uchar *s);
int GetCharIndex(char c);

int Mess_TextHeightBelowBaseline(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m;
	int v;
	int c;
	const uchar *p;

	if (!NewFont)
		return FUN_8001d548(s);

	m = 0;
	p = s;

	while (*p) {
		c = GetCharIndex(*p);

		if ((unsigned)(c - 250) < 5)
			v = 0;
		else if (c == 255)
			v = 0;
		else
			v = Font_table[Current_font].mChar_table[c].h - Font_table[Current_font].mChar_table[c].baseline;

		if (m < v)
			m = v;

		p++;
	}

	return (m * Font_table[Current_font].mFont_scale) >> 8;
}
