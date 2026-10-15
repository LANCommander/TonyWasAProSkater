/* Mess_TextHeight2__FPCc (0x8001d440) */
#include "game/m3d.h"

extern uchar BigFont_Mapping[];
extern SAnimFrame *BigFontAnim;

extern ushort Scale;

int Mess_TextHeight2(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m = 0;

	while (*s) {
		int v = BigFont_Mapping[*s];

		if (v == 254)
			v = -1;

		if (v == 253)
			v = -1;

		if (v == 252)
			v = -1;

		if (v == 251)
			v = -1;

		if (v == 250)
			v = -1;

		if (v == 255)
			v = -1;

		if (v >= 0) {
			int w = BigFontAnim[v].Height;

			if (m < w)
				m = w;
		}

		s++;
	}

	return (m * Scale) >> 8;
}
