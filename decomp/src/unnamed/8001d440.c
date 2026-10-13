/* Mess_TextHeight2__FPCc (0x8001d440) */
#include "names.h"

extern unsigned char BigFont_Mapping[];
extern unsigned char *BigFontAnim;

unsigned short Scale;

int Mess_TextHeight2(unsigned char *s) {
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
			int w = BigFontAnim[v * 8 + 3];

			if (m < w)
				m = w;
		}

		s++;
	}

	return (m * Scale) >> 8;
}
