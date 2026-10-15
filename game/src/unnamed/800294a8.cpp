/* _._6CPixel (0x800294a8) */
/* C++ (BIT.CPP). Unlinks the pixel from PixelList; CBit's destructor does the rest. */
#include "game/bit.h"

extern CPixel *PixelList;

CPixel::~CPixel() {
	DeleteFrom(&PixelList);
}
