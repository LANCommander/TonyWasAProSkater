/* SetCentreRGB__5CGlowUcUcUc (0x8002901c) */
/* C++ (BIT.CPP). Sets the colour (and the gouraud-triangle GPU code) of the glow's centre. */
#include "game/bit.h"

void CGlow::SetCentreRGB(uchar r, uchar g, uchar b) {
	mCentreCodeBGR = (0x32 << 24) | (b << 16) | (g << 8) | r;
}
