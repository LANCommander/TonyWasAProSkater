/* pack__11SlicedImage (0x800621c0) flags=-fstructure-noalias */
/* C++ (SHELLUTILS.CPP). Locals from the demo's MAIN.SYM, plus numx, which retail keeps in s5. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#include "pc_texstore.h"

void *Mem_New(uint, int, int);
void Pack_Pack(int, int, int, int *, int *, int, int, uint);

extern int Pack_NoBottom;

int SlicedImage::pack() {
	int j;
	int k;
	ushort *data_col;
	ushort *data;
	RECT r;
	int remx;
	int numy;
	int remy;
	int dum1;
	int dum2;
	int numx;

	if (is_packed)
		return 0;

	is_packed = 1;
	sliceTab = (RECT *)Mem_New(num_slices_y * num_slices_x * sizeof(RECT), 1, 1);
	data_col = slicedBuffer;
	numx = width / slice_width;
	remx = width - numx * slice_width;
	numy = height / slice_height;
	remy = height - numy * slice_height;
	Pack_NoBottom = 1;

	for (j = 0; j < numx; j++) {
		data = data_col;

		for (k = 0; k < numy; k++) {
			if (bpp == 8)
				Pack_Pack(0, slice_width * 2, slice_height, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, slice_width, slice_height, &dum1, &dum2, bpp, 0, 0);

			r.w = slice_width;
			r.h = slice_height;
			r.x = dum1;
			r.y = dum2;
			sliceTab[k * num_slices_x + j].x = dum1;
			sliceTab[k * num_slices_x + j].y = dum2;
			sliceTab[k * num_slices_x + j].w = slice_width;
			sliceTab[k * num_slices_x + j].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += slice_width * slice_height;
		}

		if (remy > 0) {
			if (bpp == 8)
				Pack_Pack(0, slice_width * 2, remy, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, slice_width, remy, &dum1, &dum2, bpp, 0, 0);

			r.h = remy;
			r.w = slice_width;
			r.x = dum1;
			r.y = dum2;
			sliceTab[numy * num_slices_x + j].x = dum1;
			sliceTab[numy * num_slices_x + j].y = dum2;
			sliceTab[numy * num_slices_x + j].w = slice_width;
			sliceTab[numy * num_slices_x + j].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}

		data_col += slice_width * height;
	}

	if (remx > 0) {
		data = data_col;

		for (k = 0; k < numy; k++) {
			if (bpp == 8)
				Pack_Pack(0, remx * 2, slice_height, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, remx, slice_height, &dum1, &dum2, bpp, 0, 0);

			r.w = remx;
			r.h = slice_height;
			r.x = dum1;
			r.y = dum2;
			sliceTab[k * num_slices_x + numx].x = dum1;
			sliceTab[k * num_slices_x + numx].y = dum2;
			sliceTab[k * num_slices_x + numx].w = remx;
			sliceTab[k * num_slices_x + numx].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += remx * slice_height;
		}

		if (remy > 0) {
			if (bpp == 8)
				Pack_Pack(0, remx * 2, remy, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, remx, remy, &dum1, &dum2, bpp, 0, 0);

			r.w = remx;
			r.h = remy;
			r.x = dum1;
			r.y = dum2;
			sliceTab[numy * num_slices_x + numx].x = dum1;
			sliceTab[numy * num_slices_x + numx].y = dum2;
			sliceTab[numy * num_slices_x + numx].w = remx;
			sliceTab[numy * num_slices_x + numx].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}
	}

	Pack_NoBottom = 0;
	return DrawSync(0);
}
