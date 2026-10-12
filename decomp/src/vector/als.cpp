/* __als__7CVectorRCi (0x8004db90) */
/* C++ (VECTOR.CPP). Shifts each component left by n. */
#include "game/vector.h"

CVector &CVector::operator<<=(const int &n) {
	vx <<= n;
	vy <<= n;
	vz <<= n;
	return *this;
}
