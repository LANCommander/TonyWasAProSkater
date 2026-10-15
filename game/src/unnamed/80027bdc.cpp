/* RemoveDeadBits__FP4CBit (0x80027bdc) */
/* C++: walks a CBit list and deletes every dead bit through its vtable (egcs-ABI virtual
   destructor, flags 3), as retail does. CBit's fields and vptr move in a 64-bit build. */
#include "game/bit.h"

/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};

void RemoveDeadBits(CBit *p) {
	CBit *n;

	if (!p)
		return;

	n = p->mNext;

	for (;;) {
		if (p->mDead != 0) {
			if (p) {
				struct VtEntry *vt = (struct VtEntry *)p->__vptr;

				vt[1].fn((char *)p + vt[1].delta, 3);
			}
		}

		p = n;

		if (n)
			n = n->mNext;
		else
			break;
	}
}
