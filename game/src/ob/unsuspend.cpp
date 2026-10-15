/* UnSuspend__5CBody (0x80043a80) */
/* C++ (OB.CPP). Moves a suspended body from the suspended list back onto the list it came from. */
#include "game/ob.h"

extern CBody *SuspendedList;

void CBody::UnSuspend() {
	if (!(mCBodyFlags & 1))
		return;

	DeleteFrom(&SuspendedList);
	AttachTo(mppOriginalList);

	mCBodyFlags &= ~1;
}
