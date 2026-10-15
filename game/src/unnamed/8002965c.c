/* Bit_SetSparkRGB__FUcUcUc (0x8002965c) */
#include "names.h"

extern char SparkRGB[4]; /* CVECTOR */

void Bit_SetSparkRGB(char a, char b, char c) {
	SparkRGB[0] = a;
	SparkRGB[1] = b;
	SparkRGB[2] = c;
}
