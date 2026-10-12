/* EntryOn__5CMenuPCc (0x80016878) */
/* C++ (FRONT.CPP). Shows the line whose text is pString. */
#include "game/front.h"

void CMenu::EntryOn(const char *pString) {
	mEntry[FindEntry(pString)].Visible = 1;
}
