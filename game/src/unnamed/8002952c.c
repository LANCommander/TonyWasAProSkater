/* Bit_SetSparkTrajectoryCone__FRC8CSVector (0x8002952c) */
#include "names.h"

struct S6 {
	short x, y, z;
};

struct S6 SparkTrajectoryCone[1];

void Bit_SetSparkTrajectoryCone(struct S6 *p) {
	SparkTrajectoryCone[0] = *p;
}
