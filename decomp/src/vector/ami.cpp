/* __ami__7CVectorRC7CVector (0x8004da78) */
/* C++ (VECTOR.CPP). Component-wise subtract. */
#include "game/vector.h"

CVector &CVector::operator-=(const CVector &v) {
	vx -= v.vx;
	vy -= v.vy;
	vz -= v.vz;
	return *this;
}
