/* __ml__FRC5CQuatT0 (0x8007722c) */
/* C++ (QUAT.CPP). Exact demo twin (the demo reaches the vtables through gp): copy b, multiply in
   place by a with the inline operator*=, return by value. Locals (tempw..tempx, inline q) from MAIN.SYM. */
#include "game/vector.h"

CQuat operator*(const CQuat &a, const CQuat &b) {
	CQuat q = b;

	q *= a;
	return q;
}
