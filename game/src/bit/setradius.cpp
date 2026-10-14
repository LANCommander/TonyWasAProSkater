/* SetRadius__5CGlowi (0x80028f94) */
/* C++ (BIT.CPP). Sets the radius of every section of the glow. */
#include "game/bit.h"

void CGlow::SetRadius(int radius) {
	for (uint i = 0; i < mNumSections; i++)
		mpSections[i].Radius = radius;
}
