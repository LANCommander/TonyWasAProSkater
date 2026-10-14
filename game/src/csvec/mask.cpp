/* Mask__8CSVector (0x8004df58) */
/* C++ (VECTOR.CPP). Masks each component (an angle) to 12 bits. */
#include "game/vector.h"

void CSVector::Mask() {
	vx &= 0xfff;
	vy &= 0xfff;
	vz &= 0xfff;
}
