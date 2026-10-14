/* SetFringeRGB__5CGlowUiUcUcUc (0x800290a4) */
/* C++ (BIT.CPP). Sets the colour (and the semi-transparent gouraud-quad GPU code) of every
   quad in one fringe ring. */
#include "game/bit.h"

void CGlow::SetFringeRGB(uint fringe, uchar r, uchar g, uchar b) {
	uint CodeBGR = (0x3a << 24) | (b << 16) | (g << 8) | r;
	SFringeQuad *pFringe = mpFringes + fringe * mNumSections;

	for (uint i = 0; i < mNumSections; i++)
		pFringe[i].CodeBGR = CodeBGR;
}
