/* Bit_SetSparkTrajectory__FRC8CSVector (0x80029508) */
#include "names.h"
typedef struct {
	short x, y, z;
} SV;

SV SparkTrajectory;

void Bit_SetSparkTrajectory(SV *v) {
	SparkTrajectory = *v;
}
