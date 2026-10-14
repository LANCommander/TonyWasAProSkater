/* CVectorRotateToPlane__FP7CVectorP7SVECTORT0 (0x80030090) */
#include "game/vector.h"

struct SVECTOR {
	short vx, vy, vz, pad;
};

int Mips_CVecDotProd(CVector *, CVector *);
int M3dMaths_SquareRoot0(int);
void ProjectCVectorToPlane(CVector *, SVECTOR);

void CVectorRotateToPlane(CVector *InVector, SVECTOR *Normal, CVector *OutVector) {
	int OldMag, NewMag;
	CVector t;

	OldMag = M3dMaths_SquareRoot0(Mips_CVecDotProd(InVector, InVector)) << 6;
	*OutVector = *InVector;
	ProjectCVectorToPlane(OutVector, *Normal);

	NewMag = M3dMaths_SquareRoot0(Mips_CVecDotProd(OutVector, OutVector)) << 6;
	OldMag >>= 8;
	NewMag >>= 8;
	__asm__("" : "=m"(t));

	*OutVector = (*OutVector * OldMag) / NewMag;
}
