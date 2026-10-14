/* Utils_CalcUnit__FR7CVectorRC7CVector (0x80047a34) */
#include <sys/types.h>
#include <libgte.h>
#ifndef PC_PORT
#include <inline_c.h>
#endif

typedef struct {
	int x, y, z;
} V;

extern long SquareRoot0(long);

static inline void VI(V *p, int _x, int _y, int _z) {
	p->x = _x;
	p->y = _y;
	p->z = _z;
}

void Utils_CalcUnit(V *Unit, V *Vec) {
	V u;
	int l;

	VI(&u, Vec->x >> 12, Vec->y >> 12, Vec->z >> 12);
#ifdef PC_PORT
	geom_square(&u, &u);
#else
	gte_ldlvl(&u);
	gte_sqr0();
	gte_stlvnl(&u);
#endif

	l = SquareRoot0(u.x + u.y + u.z);

	if (l) {
		Unit->x = Vec->x / l;
		Unit->y = Vec->y / l;
		Unit->z = Vec->z / l;
	} else {
		Unit->x = 0;
		Unit->y = 0;
		Unit->z = 0;
	}
}
