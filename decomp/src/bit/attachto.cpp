/* AttachTo__4CBitPv (0x80027d90) */
/* C++ (BIT.CPP). Pushes the bit onto the front of the doubly-linked list whose head pointer is at
   pList. */
#include "game/bit.h"

void CBit::AttachTo(void *pList) {
	CBit **ppHead = (CBit **)pList;

	mPrevious = 0;
	mNext = *ppHead;
	*ppHead = this;

	if (mNext)
		mNext->mPrevious = this;
}
