/* ProjectCVectorToPlane__FP7CVectorG7SVECTOR (0x8002fff4) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	short vx, vy, vz, pad;
} SVECTOR;

extern int Mips_CVecDotProd(V *, V *);
extern int Mips_FixMul(int, int);

void ProjectCVectorToPlane(V *v, SVECTOR n) {
	V t;
	int d;

	t.x = n.vx;
	t.y = n.vy;
	t.z = n.vz;
	d = Mips_CVecDotProd(v, &t);
	v->x -= Mips_FixMul(d, n.vx);
	v->y -= Mips_FixMul(d, n.vy);
	v->z -= Mips_FixMul(d, n.vz);
}
