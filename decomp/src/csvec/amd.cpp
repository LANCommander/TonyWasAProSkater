/* __amd__8CSVectorRC9CFriction (0x8004e074) */
/* C++ (VECTOR.CPP). Applies friction: each component loses itself >> its friction shift. */
#include "game/vector.h"

CSVector &CSVector::operator%=(const CFriction &f) {
	vx -= vx >> f.vx;
	vy -= vy >> f.vy;
	vz -= vz >> f.vz;
	return *this;
}
