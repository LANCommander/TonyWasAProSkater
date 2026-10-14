/* AttachTo__5CBodyPP5CBody (0x800438dc) */
/* C++ (OB.CPP). Pushes the body onto the front of a doubly linked object list. */
#include "game/ob.h"

void CBody::AttachTo(CBody **pp_list) {
	mPreviousItem = 0;
	mNextItem = *pp_list;
	*pp_list = this;

	if (mNextItem)
		mNextItem->mPreviousItem = this;
}
