/* removeBlack__11SlicedImage (0x800637d0) */
/* C++ (SHELLUTILS.CPP). In a 16-bit image, turns black (transparent) pixels into opaque black. */
#include "game/shellutils.h"

void SlicedImage::removeBlack() {
	if (bpp == 16) {
		int n = width * height;
		ushort *p = slicedBuffer;

		for (int i = 0; i < n; i++, p++)
			if (!*p)
				*p = 0x8000;
	}
}
