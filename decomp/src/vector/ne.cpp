/* __ne__7CVectorRC7CVector (0x8004dc5c) */
/* C++ (VECTOR.CPP). Component-wise inequality. */
#include "game/vector.h"

int CVector::operator!=(const CVector &v) {
	return vx != v.vx || vy != v.vy || vz != v.vz;
}
