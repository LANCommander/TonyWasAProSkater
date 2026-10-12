/* setString__12VerticalMenuiPc (0x800646b0) */
/* C++ (SHELLUTILS.CPP). Changes item i's text; reselects it (to redraw the triangles) if it's the
   selected item and the selection isn't animating. */
#include "game/shellutils.h"

extern "C" char *FUN_8008e904(char *, char *); /* strcpy */

void VerticalMenu::setString(int i, char *pString) {
	FUN_8008e904(list[i].string, pString);

	if (tick < 61 && i == selected)
		select(i);
}
