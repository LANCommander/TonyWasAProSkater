/* __amd__7CVectorRC9CFriction (0x8004dbcc) */
/* C++ (VECTOR.CPP). Applies friction: each component loses itself >> its friction shift. */
#include "game/vector.h"

CVector &CVector::operator%=(const CFriction &f) {
	vx -= vx >> f.vx;
	vy -= vy >> f.vy;
	vz -= vz >> f.vz;
	return *this;
}
