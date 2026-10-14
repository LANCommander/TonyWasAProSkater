/* DrawIt__FP4RECTiiii (0x800632cc) */
#include "pc_draw.h"
/* C++ (SHELLUTILS.CPP). Exact demo twin. Draws a VRAM rectangle at (x, y) as a textured quad
   (16-bit, or 8-bit with a CLUT and doubled width), then waits for the GPU.
   Levers: the demo line table gives one libgpu macro per line (setXYWH/setUVWH); `r->y & 0xff`
   in setUVWH and `<< 1` (not `* 2`) for the 8-bit widths keep both arms' registers identical so
   the getTPage tail cross-jumps as in retail. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

void DrawIt(RECT *r, int x, int y, int bpp, int clut) {
	POLY_FT4 poly;
	POLY_FT4 *apoly;

	apoly = &poly;
	setPolyFT4(apoly);
	setRGB0(apoly, 0, 0, 0);
	setShadeTex(apoly, 1);

	if (bpp == 16) {
		setXYWH(apoly, x, y, r->w, r->h);
		setUVWH(apoly, r->x & 0x3f, r->y & 0xff, r->w, r->h);

		apoly->tpage = getTPage(2, 0, r->x, r->y);
	} else {
		setXYWH(apoly, x, y, r->w << 1, r->h);
		setUVWH(apoly, (r->x & 0x3f) << 1, r->y & 0xff, r->w << 1, r->h);

		apoly->tpage = getTPage(1, 0, r->x, r->y);
		apoly->clut = clut;
	}

	PC_DrawPacket((void *)(apoly));
	DrawSync(0);
}
