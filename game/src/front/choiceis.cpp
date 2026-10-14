/* ChoiceIs__5CMenuPCc (0x8001692c) */
/* C++ (FRONT.CPP). Whether the current line's text is pString. */
#include "game/front.h"

int Utils_CompareStrings(const char *, const char *, unsigned int);

int CMenu::ChoiceIs(const char *pString) {
	return Utils_CompareStrings(pString, mEntry[mLine].pString, 0xffffffff);
}
