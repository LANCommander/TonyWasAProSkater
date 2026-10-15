/* DeleteList__FP5CBody (0x80071b00) */
/* C++: deletes every body on a list (virtual deleting destructor) except the ones with
   CBody flag 0x20 */
#include "game/ob.h"

void DeleteList(CBody *p) {
	CBody *n;

	if (!p)
		return;

	n = (CBody *)p->mNextItem;

	for (;;) {
		if (!(p->mCBodyFlags & 32))
			delete p;

		p = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}
