/* __6CPixel (0x80029468) */
/* C++ (BIT.CPP). A single-tile particle, linked into PixelList. */
#include "game/bit.h"

CPixel *PixelList;

CPixel::CPixel() {
	AttachTo(&PixelList);
}
