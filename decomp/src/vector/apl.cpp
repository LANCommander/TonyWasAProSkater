/* __apl__7CVectorRC7CVector (0x8004da3c) */
/* C++ (VECTOR.CPP). Component-wise add. */
#include "game/vector.h"

CVector &CVector::operator+=(const CVector &v) {
	vx += v.vx;
	vy += v.vy;
	vz += v.vz;
	return *this;
}
