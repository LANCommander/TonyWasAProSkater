/* Suspend__5CBodyPP5CBody (0x80043a14) */
/* C++ (OB.CPP). Frees the body's extras (virtual DeleteStuff), then moves it from its list onto
   the suspended list, remembering the list to return to. */
#include "game/ob.h"

extern CBody *SuspendedList;

void CBody::Suspend(CBody **pp_list) {
	DeleteStuff();

	mppOriginalList = pp_list;
	DeleteFrom(pp_list);
	AttachTo(&SuspendedList);

	mCBodyFlags |= 1;
}
