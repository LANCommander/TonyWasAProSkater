/* DeleteFrom__5CBodyPP5CBody (0x80043908) */
/* C++ (OB.CPP). Unlinks the body from a list (taking it off the suspended list first if it is
   suspended and the list is another one). */
#include "game/ob.h"

CBody *SuspendedList;

void CBody::DeleteFrom(CBody **pp_list) {
	if ((mCBodyFlags & 1) && pp_list != &SuspendedList)
		UnSuspend();

	if (mNextItem)
		mNextItem->mPreviousItem = mPreviousItem;

	if (mPreviousItem)
		mPreviousItem->mNextItem = mNextItem;

	if (*pp_list == this)
		*pp_list = (CBody *)mNextItem;
}
