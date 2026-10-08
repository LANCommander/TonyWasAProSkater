/* Bit_CalculateSparkVelocity__FR7CVectori (0x80029550) */
#include "names.h"
typedef struct {
	int x, y, z;
} V;

typedef struct {
	short x, y, z;
} SV;

SV SparkTrajectory;
short SparkTrajectoryCone, SparkTrajectoryCone_2, SparkTrajectoryCone_4;

int Redbook_Rnd(int);
void Utils_GetVecFromMagDir(V *, int, SV *);

void Bit_CalculateSparkVelocity(V *out, int mag) {
	SV d = SparkTrajectory;

	if (SparkTrajectoryCone)
		d.x += Redbook_Rnd(SparkTrajectoryCone) - (SparkTrajectoryCone >> 1);

	if (SparkTrajectoryCone_2)
		d.y += Redbook_Rnd(SparkTrajectoryCone_2) - (SparkTrajectoryCone_2 >> 1);

	if (SparkTrajectoryCone_4)
		d.z += Redbook_Rnd(SparkTrajectoryCone_4) - (SparkTrajectoryCone_4 >> 1);

	Utils_GetVecFromMagDir(out, mag, &d);
}
