/* AddEntry__5CMenuPCc (0x800167f0) */
/* C++ (FRONT.CPP). Appends a visible, enabled line with no extra clearance. */
#include "game/front.h"

void CMenu::AddEntry(const char *pString) {
	mEntry[mNumLines].pString = (char *)pString;
	mEntry[mNumLines].Visible = 1;
	mEntry[mNumLines].Enabled = 1;
	mEntry[mNumLines].Clearance = 0;
	mNumLines++;
}
