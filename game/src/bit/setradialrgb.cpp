/* SetRadialRGB__5CGlowiUcUcUc (0x80029110) */
/* C++ (BIT.CPP). Sets the colour of one radial spoke: its outer section and the matching
   quad of every fringe ring. */
#include "game/bit.h"

void CGlow::SetRadialRGB(int section, uchar r, uchar g, uchar b) {
	uint CodeBGR = (0x3a << 24) | (b << 16) | (g << 8) | r;
	SFringeQuad *pFringe;

	mpSections[section].CodeBGR = CodeBGR;
	pFringe = mpFringes + section;

	for (uint i = 0; i < mNumFringes; i++) {
		pFringe->CodeBGR = CodeBGR;
		pFringe += mNumSections;
	}
}
