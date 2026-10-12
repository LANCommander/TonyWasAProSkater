/* SetFringeWidth__5CGlowUiUi (0x80028fd4) */
/* C++ (BIT.CPP). Sets the width of every quad in one fringe ring. */
#include "game/bit.h"

void CGlow::SetFringeWidth(uint fringe, uint width) {
	SFringeQuad *pFringe = mpFringes + fringe * mNumSections;

	for (uint i = 0; i < mNumSections; i++)
		pFringe[i].Width = width;
}
