/* KillSmall__7CVector (0x8004d9e0) */
/* C++ (VECTOR.CPP). Zeroes each component in [-0x800, 0x800]. */
#include "game/vector.h"

void CVector::KillSmall() {
	if ((unsigned)(vx + 0x800) < 0x1001)
		vx = 0;

	if ((unsigned)(vy + 0x800) < 0x1001)
		vy = 0;

	if ((unsigned)(vz + 0x800) < 0x1001)
		vz = 0;
}
