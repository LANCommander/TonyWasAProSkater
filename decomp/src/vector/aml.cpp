/* __aml__7CVectorRCi (0x8004dab4) */
/* C++ (VECTOR.CPP). Multiplies each component by n. */
#include "game/vector.h"

CVector &CVector::operator*=(const int &n) {
	vx *= n;
	vy *= n;
	vz *= n;
	return *this;
}
