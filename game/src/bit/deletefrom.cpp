/* DeleteFrom__4CBitPv (0x80027dbc) */
/* C++ (BIT.CPP). Unlinks the bit from the doubly-linked list whose head pointer is at pList. */
#include "game/bit.h"

void CBit::DeleteFrom(void *pList) {
	CBit **ppHead = (CBit **)pList;

	if (mNext)
		mNext->mPrevious = mPrevious;

	if (mPrevious)
		mPrevious->mNext = mNext;

	if (*ppHead == this)
		*ppHead = mNext;
}
