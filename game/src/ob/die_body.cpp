/* Die__5CBody (0x80043cac) */
/* C++ (OB.CPP). Marks the body dead (once). */
#include "game/ob.h"

void CBody::Die() {
	if (!IsDead())
		mCBodyFlags |= 0x40;
}
