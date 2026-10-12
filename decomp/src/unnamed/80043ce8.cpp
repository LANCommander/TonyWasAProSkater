/* IsDead__C5CBody (0x80043ce8) */
/* C++ (OB.CPP). Whether the body has been marked dead. */
#include "game/ob.h"

int CBody::IsDead() const {
	return (mCBodyFlags & 0x40) != 0;
}
