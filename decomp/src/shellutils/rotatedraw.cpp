/* rotateDraw__11SlicedImageii (0x80063474) */
/* C++ (SHELLUTILS.CPP). Demo twin; names from its MAIN.SYM. Uploads the image one slice at a time
   into a single VRAM RECT (allocated on first use) and draws each slice rotated via DrawIt.
   numx/remy are declared mid-block, at their first assignment: their pseudos are then created
   after the division's quotient temp, which gives retail's spill-slot order (numx 48, remy 52). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#ifdef PC_PORT
#include "pc_texstore.h"
#endif

void *Mem_New(uint, int, int);
void Pack_Pack(int, int, int, int *, int *, int, int, uint);
void DrawIt(RECT *r, int x, int y, int bpp, int clut);

extern int Pack_NoBottom;

void SlicedImage::rotateDraw(int x, int y) {
	RECT *tab = sliceTab;
	RECT r;
	int dum1, dum2;
	int w, h, sw, sh;
	int numy, remx, j, k;
	ushort *data_col, *data;
	int xt, yt;

	if (!tab) {
		sliceTab = (RECT *)Mem_New(sizeof(RECT), 1, 1);
		Pack_Pack(0, slice_width, slice_height, &dum1, &dum2, 16, 0, 0);

		sliceTab->w = slice_width;
		sliceTab->h = slice_height;
		sliceTab->x = dum1;
		sliceTab->y = dum2;
		tab = sliceTab;
	}

	w = width;
	sw = slice_width;
	int numx = w / sw;

	h = height;
	sh = slice_height;
	numy = h / sh;
	r.h = tab->h;
	r.w = tab->w;
	r.x = tab->x;
	r.y = tab->y;
	data_col = slicedBuffer;
	xt = x;
	Pack_NoBottom = 1;
	remx = w - numx * sw;
	int remy = h - numy * sh;

	for (j = 0; j < numx; j++) {
		data = data_col;
		yt = y;

		for (k = 0; k < numy; k++) {
			r.w = slice_width;
			r.h = slice_height;
#ifdef PC_PORT
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
#else
			LoadImage(&r, (u_long *)data);
#endif
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);

			yt += slice_height;
			data += slice_width * slice_height;
		}

		if (remy > 0) {
			r.w = slice_width;
			r.h = remy;
#ifdef PC_PORT
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
#else
			LoadImage(&r, (u_long *)data);
#endif
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);
		}

		data_col += slice_width * height;

		if (bpp == 16)
			xt += slice_width;
		else
			xt += slice_width * 2;
	}

	if (remx > 0) {
		data = data_col;
		yt = y;

		for (k = 0; k < numy; k++) {
			r.w = remx;
			r.h = slice_height;
#ifdef PC_PORT
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
#else
			LoadImage(&r, (u_long *)data);
#endif
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);

			yt += slice_height;
			data += remx * slice_height;
		}

		if (remy > 0) {
			r.w = remx;
			r.h = remy;
#ifdef PC_PORT
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
#else
			LoadImage(&r, (u_long *)data);
#endif
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);
		}
	}

	Pack_NoBottom = 0;
}
