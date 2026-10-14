/* __11SlicedImagePciiiiiUs (0x80061ef4) */
/* C++ (SHELLUTILS.CPP). Copies a w x h image (8-bit images count 16-bit words, so half width) into
   a buffer laid out slice column by slice column (slices of sw x sh; 0 = whole image), plus a
   narrower last column for the remainder, then frees the source pixels. */
#include "game/shellutils.h"

void *Mem_NewX(unsigned int, int, int);
void Mem_Delete(void *);

SlicedImage::SlicedImage(char *pPixels, int w, int h, int sw, int sh, int depth, ushort clutid) {
	ushort *d, *s;
	int rest;

	is_packed = 0;
	bpp = depth;
	clut = clutid;

	if (depth == 8)
		w = w >> 1;

	if (sw == 0)
		sw = w;

	if (sh == 0)
		sh = h;

	width = w;
	height = h;
	slice_width = sw;
	slice_height = sh;
	sliceTab = 0;
	num_slices_x = w / sw;
	num_slices_y = (h + sh - 1) / sh;
	slicedBuffer = (ushort *)Mem_NewX(w * h * 2, 1, 1);

	d = slicedBuffer;
	s = (ushort *)pPixels;

	for (int i = 0; i < num_slices_x; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < slice_width; k++)
				*d++ = *s++;

			s += width - slice_width;
		}

		s += slice_width - width * height;
	}

	rest = width - num_slices_x * slice_width;

	if (rest > 0) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < rest; k++)
				*d++ = *s++;

			s += width - rest;
		}

		num_slices_x++;
	}

	Mem_Delete(pPixels);

	m_removed_from_memory = 0;
}
