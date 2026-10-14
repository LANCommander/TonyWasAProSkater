/* Panel_SetStretchedScreenCoords__FiiP8POLY_FT4P10SAnimFrame (0x8006c594) */
#include <sys/types.h>
#include <libgpu.h>

typedef struct {
	signed char xoff, yoff;
	unsigned char w, h;
} SAnimFrame;

void Panel_SetStretchedScreenCoords(int x, int y, POLY_FT4 *p, SAnimFrame *f) {
	int x0 = x + (f->xoff << 9) / 320;
	int x1 = x0 + (f->w << 9) / 320;
	int y0 = y + f->yoff;
	int y1 = y0 + f->h;

	p->y0 = y0;
	p->y1 = y0;
	p->y2 = y1;
	p->y3 = y1;
	p->x0 = x0;
	p->x1 = x1;
	p->x2 = x0;
	p->x3 = x1;
}
