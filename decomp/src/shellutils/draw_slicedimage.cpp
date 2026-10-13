/* draw__11SlicedImageiiiiiiii (0x80062e20) */
/* C++ (SHELLUTILS.CPP). Emits one POLY_FT4 per slice of the image at (x, y), flat (r == -1) or
   tinted, optionally semi-transparent, into the 2D OT (otFront) or successive OT layers counting
   down from OT_entry, or draws it at once (drawNow). The tpage is getTPage on the slice RECT. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#include "game/m3d.h"

extern char s_OT_entry_is_d[];
extern SDoubleBuffer *pDoubleBuffer;
extern char *pPoly;

int OT_entry;

extern "C" {
	int FUN_8008c190(const char *, ...); /* printf */
	int FUN_8008c880(void *);            /* DrawPrim */
}

void SlicedImage::draw(int x, int y, int otFront, int r, int g, int b, int semiTrans, int drawNow) {
	drawScaled(x, y, 0, 0, otFront, r, g, b, semiTrans, drawNow);
}

/* w x h: the size on screen (0 = the image's own; a Sprite of an image the Dreamcast redrew
   larger is laid out at the PS1's size, platform/pc/include/pc_assets.h). Slice edges are placed
   from their offsets in the image, so scaled slices still meet. */
void SlicedImage::drawScaled(int x, int y, int w, int h, int otFront, int r, int g, int b,
                             int semiTrans, int drawNow) {
	int slice = 0;
	int shift = bpp == 8;
	u_long *ot;

	if ((u_int)OT_entry >= 0x2000)
		FUN_8008c190(s_OT_entry_is_d, OT_entry);

	if (otFront)
		ot = &pDoubleBuffer->OT2D;
	else
		ot = &pDoubleBuffer->OT3D[OT_entry];

	/* the image's size in pixels: its first row and column of slices */
	int iw = 0;
	int ih = 0;

	for (int j = 0; j < num_slices_x; j++)
		iw += sliceTab[j].w << shift;

	for (int i = 0; i < num_slices_y; i++)
		ih += sliceTab[i * num_slices_x].h;

	if (w <= 0 || h <= 0 || !iw || !ih) {
		w = iw;
		h = ih;
	}

	int oy = 0;

	for (int i = 0; i < num_slices_y; i++) {
		int ox = 0;
		int sh = sliceTab[i * num_slices_x].h;
		int y0 = y + oy * h / ih;
		int y1 = y + (oy + sh) * h / ih;

		for (int j = 0; j < num_slices_x; j++) {
			RECT *pSlice = &sliceTab[slice];
			POLY_FT4 *p = (POLY_FT4 *)pPoly;

			pPoly += sizeof(POLY_FT4);
			setlen(p, 9);

			p->code = 0x2c;

			if (r == -1) {
				p->r0 = 0;
				p->g0 = 0;
				p->b0 = 0;
			} else {
				p->r0 = r;
				p->g0 = g;
				p->b0 = b;
			}

			int x0 = x + ox * w / iw;
			int x1 = x + (ox + (pSlice->w << shift)) * w / iw;

			p->x0 = x0;
			p->y0 = y0;
			p->x1 = x1;
			p->y1 = y0;
			p->x2 = x0;
			p->y2 = y1;
			p->x3 = x1;
			p->y3 = y1;
			p->u0 = (pSlice->x & 63) << shift;
			p->v0 = pSlice->y;
			p->u1 = ((pSlice->x & 63) << shift) + (pSlice->w << shift);
			p->v1 = pSlice->y;
			p->u2 = (pSlice->x & 63) << shift;
			p->v2 = pSlice->h + pSlice->y;
			p->u3 = ((pSlice->x & 63) << shift) + (pSlice->w << shift);
			p->v3 = pSlice->h + pSlice->y;

			if (semiTrans)
				p->code |= 2;
			else
				p->code &= ~2;

			if (r == -1)
				p->code |= 1;
			else
				p->code &= ~1;

			p->tpage = getTPage(bpp == 8 ? 1 : 2, 0, pSlice->x, pSlice->y);
			p->clut = clut;

			if (!drawNow)
				addPrim(ot, p);
			else
				FUN_8008c880(p);

			if (ot != &pDoubleBuffer->OT2D)
				OT_entry--;

			slice++;
			ox += pSlice->w << shift;
		}

		oy += sh;
	}
}
