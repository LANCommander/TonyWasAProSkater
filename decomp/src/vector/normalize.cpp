/* Normalize__7CVector (0x8004d980) */
/* C++ (VECTOR.CPP). Divides by Length(). Retail never reloads z: it stores (y/len)/len there (an
   original bug). */
#include "game/vector.h"

void CVector::Normalize() {
	int l = Length();

	vx = vx / l;
	vy = vy / l;
	vz = vy / l;
}
