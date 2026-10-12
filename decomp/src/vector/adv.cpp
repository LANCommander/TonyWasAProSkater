/* __adv__7CVectorRCi (0x8004db04) */
/* C++ (VECTOR.CPP). Divides each component by n. */
#include "game/vector.h"

CVector &CVector::operator/=(const int &n) {
	vx /= n;
	vy /= n;
	vz /= n;
	return *this;
}
