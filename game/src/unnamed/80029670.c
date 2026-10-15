/* Bit_SetSparkFadeRGB__FUcUcUc (0x80029670) */
#include "names.h"

extern char SparkFadeRGB[4]; /* CVECTOR */

void Bit_SetSparkFadeRGB(char a, char b, char c) {
	SparkFadeRGB[0] = a;
	SparkFadeRGB[1] = b;
	SparkFadeRGB[2] = c;
}
