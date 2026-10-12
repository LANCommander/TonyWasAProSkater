/* EntryOff__5CMenuPCc (0x800168b4) */
/* C++ (FRONT.CPP). Hides the line whose text is pString. */
#include "game/front.h"

void CMenu::EntryOff(const char *pString) {
	mEntry[FindEntry(pString)].Visible = 0;
}
