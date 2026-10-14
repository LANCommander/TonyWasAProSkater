/* __mi__C7CVector (0x8004df2c) */
/* C++ (VECTOR.CPP). Unary minus: returns the negated vector. */
#include "game/vector.h"

CVector CVector::operator-() const {
	CVector r;
	int x = -vx, y = -vy, z = -vz;

	r.vx = x;
	r.vy = y;
	r.vz = z;
	return r;
}
