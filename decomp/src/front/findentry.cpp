/* FindEntry__5CMenuPCc (0x80016770) */
/* C++ (FRONT.CPP). Index of the line whose text is pString (0 if none). */
#include "game/front.h"

int Utils_CompareStrings(const char *, const char *, unsigned int);

int CMenu::FindEntry(const char *pString) {
	for (int i = 0; i < mNumLines; i++)
		if (Utils_CompareStrings(pString, mEntry[i].pString, 0xffffffff))
			return i;

	return 0;
}
