/* Bit_SetSparkSize__FUi (0x80029648) */
#include "names.h"

extern int SparkSize;

void Bit_SetSparkSize(int a) {
	SparkSize = (a << 16) | a;
}
