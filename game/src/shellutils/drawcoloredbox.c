/* DrawColoredBox__FiiiiUcUcUc (0x80061d44) flags=-fstructure-noalias */
#include "names.h"
/* SHELLUTILS.CPP, as C (demo twin; locals from its MAIN.SYM). A flat-shaded rectangle added to the
   current frame's per-call layer OT (which counts down), like SetClippingBox. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "game/dbuffer.h"
extern u_char *pPoly;

extern int OT_entry;

void DrawColoredBox(int x, int y, int w, int h, u_char r, u_char g, u_char b) {
	POLY_F4 *Poly = (POLY_F4 *)pPoly;

	pPoly = (u_char *)(Poly + 1);
	setPolyF4(Poly);
	setRGB0(Poly, r, g, b);
	setXY4(Poly, x, y, x + w, y, x, y + h, x + w, y + h);
	addPrim(&pDoubleBuffer->OT3D[OT_entry], Poly);

	OT_entry--;
}
