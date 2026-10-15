/* GetCharIndex__Fc (0x8001ddfc) */

/* MESS.CPP. Demo twin (THPS1 drops the bracket remaps). Maps a character to its font glyph index:
   digits and ':' only for the small numeric fonts, else letters, digits and punctuation. */
#include "game/font.h"

extern FontInfo Font_table[];

int Current_font;

int GetCharIndex(char ch) {
	unsigned c;
	int v, r;

	c = ch & 255;

	if (Font_table[Current_font].mNum_chars < 15) {
		v = c - 48;

		if ((unsigned char)v < 10)
			return v;

		if (c == 58)
			return 10;

		return 0;
	}

	if (c == 32)
		return 255;

	if (c == 91)
		return 254;

	if (c == 93)
		return 253;

	if (c == 123)
		return 254;

	if (c == 125)
		return 252;

	if (c == 60)
		return 254;

	if (c == 62)
		return 251;

	if (c == 94)
		return 250;

	v = c - 65;

	if ((unsigned char)v < 26)
		return v;

	v = c - 97;

	if ((unsigned char)v < 26)
		return v;

	if (c - 48 < 10)
		return c - 22;

	switch (c) {
		case 33:
			return 37;
		case 58:
			return 38;
		case 46:
			return 39;
		case 44:
			return 40;
		case 45:
		case 95:
			return 41;
		case 37:
			return 42;
		case 43:
			return 43;
		case 39: {
			if (Font_table[Current_font].mNum_chars >= 45)
				return 44;
		}
			return 36;
		case 63:
			return 36;
		case 35:
			return 45;
		case 34:
			return 46;
		case 38:
			return 47;
		case 64:
			return 48;
		default:
			r = 36;
			break;
	}

	return r;
}
