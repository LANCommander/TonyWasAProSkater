/* IsEntryOn__5CMenuPCc (0x800168ec) */
/* C++ (FRONT.CPP). Whether the line whose text is pString is shown. */
#include "game/front.h"

int CMenu::IsEntryOn(const char *pString) {
	return mEntry[FindEntry(pString)].Visible == 1;
}
