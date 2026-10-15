/* Mess_TextHeightAboveBaseline__FPCc (0x8001dc9c) */

#include "game/font.h"

extern FontInfo Font_table[];

uchar NewFont;
int Current_font;

int Mess_TextHeightAboveBaseline2(const char *s);
int GetCharIndex(char c);

int Mess_TextHeightAboveBaseline(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m;
	int v;
	int c;
	const uchar *p;

	if (!NewFont)
		return Mess_TextHeightAboveBaseline2(s0);

	m = 0;
	p = s;

	while (*p) {
		c = GetCharIndex(*p);

		if ((unsigned)(c - 250) < 5)
			v = 0;
		else if (c == 255)
			v = 0;
		else
			v = Font_table[Current_font].mChar_table[c].baseline;

		if (m < v)
			m = v;

		p++;
	}

	return (m * Font_table[Current_font].mFont_scale) >> 8;
}
