/* Mick_Darken__Fv (0x80024454) */
/* C++ (MAIN.CPP). Fuzzy demo twin; locals from its MAIN.SYM. Fades a full-screen semi-transparent
   black-ish quad in (brightness climbs by 4 a frame up to 40). The corners are set in the demo's
   source-line grouping (x0/x1, y0/y1, x2/x3, y2/y3); setXY4's order schedules differently. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;

uchar *PolyBufferEnd;
int Mick_Darkness;

void Mick_Darken() {
	POLY_F4 *aPoly;
	DR_TPAGE *ptpage;
	uchar *newppoly;
	u_long *ot = &pDoubleBuffer->OT;

	if (Mick_Darkness < 40)
		Mick_Darkness += 4;

	aPoly = (POLY_F4 *)pPoly;
	newppoly = (uchar *)aPoly + sizeof(POLY_F4) + sizeof(DR_TPAGE);
	ptpage = (DR_TPAGE *)(aPoly + 1);

	if (newppoly > PolyBufferEnd)
		return;

	pPoly = newppoly;
	setPolyF4(aPoly);
	setSemiTrans(aPoly, 1);
	setRGB0(aPoly, Mick_Darkness, Mick_Darkness, Mick_Darkness);

	aPoly->x0 = 0;
	aPoly->x1 = 512;
	aPoly->y0 = 0;
	aPoly->y1 = 0;
	aPoly->x2 = 0;
	aPoly->x3 = 512;
	aPoly->y2 = 240;
	aPoly->y3 = 240;
	addPrim(ot, aPoly);

	ptpage->tag = 0x01000000;
	ptpage->code[0] = _get_mode(1, 1, 0x40);
	addPrim(ot, ptpage);
}
