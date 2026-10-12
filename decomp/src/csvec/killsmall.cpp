/* KillSmall__8CSVector (0x8004df80) */
/* C++ (VECTOR.CPP). Zeroes each component that is -1, 0 or 1. */
#include "game/vector.h"

void CSVector::KillSmall() {
	if ((unsigned short)(vx + 1) < 3)
		vx = 0;

	if ((unsigned short)(vy + 1) < 3)
		vy = 0;

	if ((unsigned short)(vz + 1) < 3)
		vz = 0;
}
