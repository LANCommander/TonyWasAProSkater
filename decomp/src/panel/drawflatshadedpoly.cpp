/* Panel_DrawFlatShadedPoly__FiiiiUcUcUci (0x8006c610) flags=-fstructure-noalias */
/* C++ (PANEL.CPP). Demo twin; locals from its MAIN.SYM. Adds a flat-shaded rectangle to the
   ordering table at the given depth; returns the primitive, or 0 if the primitive buffer is full. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

POLY_F4 *Panel_DrawFlatShadedPoly(int x, int y, int w, int h, uchar r, uchar g, uchar b, int sort) {
	POLY_F4 *apoly = (POLY_F4 *)pPoly;
	uchar *newppoly = pPoly + sizeof(POLY_F4);

	if (newppoly > PolyBufferEnd)
		return 0;

	pPoly = newppoly;
	setPolyF4(apoly);
	setRGB0(apoly, r, g, b);
	setXY4(apoly, x, y, x + w, y, x, y + h, x + w, y + h);
	addPrim(&(&pDoubleBuffer->OT2D)[sort], apoly);
	return apoly;
}
