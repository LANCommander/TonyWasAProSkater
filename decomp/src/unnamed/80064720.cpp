/* _._12VerticalMenu (0x80064720) */
/* C++ (SHELLUTILS.CPP). Deletes the selection triangles (unless they're shared), their erase
   sprites and every item's highlight bar. */
#include "game/shellutils.h"

VerticalMenu::~VerticalMenu() {
	if (!mDontDeleteSprites) {
		delete mTri1;
		delete mTri2;
	}

	delete mEraseTri1;
	delete mEraseTri2;

	for (int i = 0; i < num_items; i++)
		delete list[i].erase_sprite;
}
