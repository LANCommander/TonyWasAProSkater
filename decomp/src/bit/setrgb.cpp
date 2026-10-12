/* SetRGB__5CGlowUcUcUc (0x80029048) */
/* C++ (BIT.CPP). Sets the colour of every outer section of the glow. */
#include "game/bit.h"

void CGlow::SetRGB(uchar r, uchar g, uchar b) {
	uint CodeBGR = (b << 16) | (g << 8) | r;

	for (uint i = 0; i < mNumSections; i++)
		mpSections[i].CodeBGR = CodeBGR;
}
