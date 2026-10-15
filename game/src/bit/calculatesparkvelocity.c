/* Bit_CalculateSparkVelocity__FR7CVectori (0x80029550) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short x, y, z;
} SV;

extern SV SparkTrajectory;
extern short SparkTrajectoryCone[4]; /* SVECTOR */

int Redbook_Rnd(int);
void Utils_GetVecFromMagDir(V *, int, SV *);

void Bit_CalculateSparkVelocity(V *out, int mag) {
	SV d = SparkTrajectory;

	if (SparkTrajectoryCone[0])
		d.x += Redbook_Rnd(SparkTrajectoryCone[0]) - (SparkTrajectoryCone[0] >> 1);

	if (SparkTrajectoryCone[1])
		d.y += Redbook_Rnd(SparkTrajectoryCone[1]) - (SparkTrajectoryCone[1] >> 1);

	if (SparkTrajectoryCone[2])
		d.z += Redbook_Rnd(SparkTrajectoryCone[2]) - (SparkTrajectoryCone[2] >> 1);

	Utils_GetVecFromMagDir(out, mag, &d);
}
