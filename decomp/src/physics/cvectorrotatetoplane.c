/* CVectorRotateToPlane__FP7CVectorP7SVECTORT0 (0x80030090) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short vx, vy, vz, pad;
} SVECTOR;

extern int Mips_CVecDotProd(V *, V *);
extern long M3dMaths_SquareRoot0__Fi(long);
extern void ProjectCVectorToPlane(V *, SVECTOR);
extern V __ml__FRC7CVectorRCi(const V *, const int *);
extern V __dv__FRC7CVectorRCi(const V *, const int *);

void CVectorRotateToPlane(V *v, SVECTOR *n, V *out) {
	int l1, l0;
	V a;
	V b;
	SVECTOR nn;

	l0 = M3dMaths_SquareRoot0__Fi(Mips_CVecDotProd(v, v)) << 6;
	*out = *v;
	nn = *n;
	ProjectCVectorToPlane(out, nn);

	l1 = M3dMaths_SquareRoot0__Fi(Mips_CVecDotProd(out, out)) << 6;
	l1 >>= 8;
	l0 >>= 8;
	a = __ml__FRC7CVectorRCi(out, &l0);
	b = __dv__FRC7CVectorRCi(&a, &l1);
	*out = b;
}
