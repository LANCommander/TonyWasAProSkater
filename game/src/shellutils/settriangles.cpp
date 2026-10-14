/* setTriangles__12VerticalMenuP6SpriteT1 (0x80064890) */
/* C++ (SHELLUTILS.CPP). Uses caller-owned selection triangles (so the menu won't delete them),
   keeping any already set. */
#include "game/shellutils.h"

void VerticalMenu::setTriangles(Sprite *tri1, Sprite *tri2) {
	mDontDeleteSprites = 1;

	if (!mTri1)
		mTri1 = tri1;

	if (!mTri2)
		mTri2 = tri2;
}
