/* CalcRGB__FiUiiPUi (0x8002172c) */
#include "names.h"

unsigned char AverageR, AverageG, AverageB;

void CalcRGB(int f, unsigned c, int mode, unsigned *t) {
	unsigned a, b, d;
	int r, g, bl;

	if (f) {
		a = t[c & 255];
		b = t[(c >> 8) & 255];
		d = t[(c >> 16) & 255];
		r = (a & 255) + (b & 255) + (d & 255);
		g = ((a >> 8) & 255) + ((b >> 8) & 255) + ((d >> 8) & 255);
		bl = ((a >> 16) & 255) + ((b >> 16) & 255) + ((d >> 16) & 255);

		if (mode == 4) {
			unsigned e = t[c >> 24];

			r += e & 255;
			g += (e >> 8) & 255;
			bl += (e >> 16) & 255;
			AverageR = r / 4;
			AverageG = g / 4;
			AverageB = bl / 4;
		} else {
			AverageR = r / 3;
			AverageG = g / 3;
			AverageB = bl / 3;
		}
	} else {
		AverageR = c;
		AverageG = c >> 8;
		AverageB = c >> 16;
	}
}
