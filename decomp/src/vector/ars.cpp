/* __ars__7CVectorRCi (0x8004db54) */
/* C++ (VECTOR.CPP). Shifts each component right (arithmetic) by n. */
#include "game/vector.h"

CVector &CVector::operator>>=(const int &n) {
	vx >>= n;
	vy >>= n;
	vz >>= n;
	return *this;
}
