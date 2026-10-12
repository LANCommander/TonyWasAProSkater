/* Length__C7CVector (0x8004d900) */
/* C++ (VECTOR.CPP). GTE SQR of the >>12 components, then SquareRoot0 (0x8008b5dc). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/vector.h"

int CVector::Length() const {
	VECTOR t;

	t.vx = vx >> 12;
	t.vy = vy >> 12;
	t.vz = vz >> 12;
	gte_ldlvl(&t);
	gte_sqr0();
	gte_stlvnl(&t);
	return SquareRoot0(t.vx + t.vy + t.vz);
}
