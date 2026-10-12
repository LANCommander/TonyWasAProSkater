/* unpack__11SlicedImage (0x80062d00) */
/* C++ (SHELLUTILS.CPP). Releases the image's VRAM: every slice if it was packed, else the single
   slice (if any); then frees the slice table. */
#include "game/shellutils.h"

void Pack_Unpack(short, short);
void Mem_Delete(void *);

void SlicedImage::unpack() {
	if (!is_packed) {
		if (!sliceTab)
			return;

		Pack_Unpack(sliceTab->x, sliceTab->y);
	} else {
		is_packed = 0;

		for (int i = 0; i < num_slices_y; i++)
			for (int j = 0; j < num_slices_x; j++)
				Pack_Unpack(sliceTab[i * num_slices_x + j].x, sliceTab[i * num_slices_x + j].y);
	}

	Mem_Delete(sliceTab);
	sliceTab = 0;
}
