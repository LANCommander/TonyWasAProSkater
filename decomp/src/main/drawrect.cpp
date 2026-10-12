/* Draw_Rect__Fiiiiiiiii (0x80024e98) flags=-fstructure-noalias */
/* C++ (MAIN.CPP). Demo twin; locals from its MAIN.SYM. Adds a flat quad with the given corners
   (right and bottom edges inclusive) and packed colour word to ordering-table slot 2. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;

void Draw_Rect(int X0, int Y0, int X1, int Y1, int X2, int Y2, int X3, int Y3, int c) {
	POLY_F4 *pF4 = (POLY_F4 *)pPoly;
	uchar *newppoly;

	*(u_long *)&pF4->r0 = c;
	pF4->x0 = X0;
	pF4->y0 = Y0;
	pF4->x1 = X1 + 1;
	pF4->y1 = Y1;
	pF4->x3 = X2 + 1;
	pF4->y3 = Y2 + 1;
	pF4->x2 = X3;
	pF4->y2 = Y3 + 1;
	newppoly = (uchar *)pF4 + sizeof(POLY_F4);
	SetPolyF4((POLY_F4 *)pPoly);
	addPrim(&pDoubleBuffer->OT3D[0], pPoly);

	pPoly = newppoly;
}
