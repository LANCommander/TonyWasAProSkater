/* setString__12VerticalMenuiPc (0x800646b0) */
/* C++ (SHELLUTILS.CPP). Changes item i's text; reselects it (to redraw the triangles) if it's the
   selected item and the selection isn't animating. */
#include "game/psyq.h"
#include "game/shellutils.h"

void VerticalMenu::setString(int i, char *pString) {
	strcpy(list[i].string, pString);

	if (tick < 61 && i == selected)
		select(i);
}
