/* DrawGouraudPoly__FiiiiUcUcUcUcUcUci (0x8006c6e4) flags=-fstructure-noalias */
/* C++ (PANEL.CPP). Demo twin; locals from its MAIN.SYM. Adds a Gouraud-shaded rectangle (left
   edge in the bottom colour, right edge in the top colour) to the ordering table at the given
   depth; returns the primitive, or 0 if the primitive buffer is full. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

POLY_G4 *DrawGouraudPoly(int x, int y, int w, int h, uchar rtop, uchar gtop, uchar btop,
                         uchar rbottom, uchar gbottom, uchar bbottom, int sort) {
	POLY_G4 *apoly = (POLY_G4 *)pPoly;
	uchar *newppoly = pPoly + sizeof(POLY_G4);

	if (newppoly > PolyBufferEnd)
		return 0;

	pPoly = newppoly;
	setPolyG4(apoly);

	apoly->r0 = rbottom;
	apoly->r1 = rtop;
	apoly->g0 = gbottom;
	apoly->g1 = gtop;
	apoly->b0 = bbottom;
	apoly->b1 = btop;
	apoly->r2 = rbottom;
	apoly->r3 = rtop;
	apoly->g2 = gbottom;
	apoly->g3 = gtop;
	apoly->b2 = bbottom;
	apoly->b3 = btop;
	apoly->x0 = x;
	apoly->x1 = x + w;
	apoly->y0 = y;
	apoly->y1 = y;
	apoly->x2 = x;
	apoly->x3 = x + w;
	apoly->y2 = y + h;
	apoly->y3 = y + h;
	addPrim(&(&pDoubleBuffer->OT2D)[sort], apoly);
	return apoly;
}
