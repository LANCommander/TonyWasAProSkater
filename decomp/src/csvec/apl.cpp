/* __apl__8CSVectorRC8CSVector (0x8004dfe8) */
/* C++ (VECTOR.CPP). Component-wise add (truncated to short). */
#include "game/vector.h"

CSVector &CSVector::operator+=(const CSVector &v) {
	vx += v.vx;
	vy += v.vy;
	vz += v.vz;
	return *this;
}
