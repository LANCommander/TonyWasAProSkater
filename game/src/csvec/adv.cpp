/* __adv__8CSVectorRCi (0x8004e024) */
/* C++ (VECTOR.CPP). Divides each component by n (truncated back to short). */
#include "game/vector.h"

CSVector &CSVector::operator/=(const int &n) {
	vx /= n;
	vy /= n;
	vz /= n;
	return *this;
}
