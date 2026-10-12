/* __eq__7CVectorRC7CVector (0x8004dc18) */
/* C++ (VECTOR.CPP). Component-wise equality. */
#include "game/vector.h"

int CVector::operator==(const CVector &v) {
	return vx == v.vx && vy == v.vy && vz == v.vz;
}
