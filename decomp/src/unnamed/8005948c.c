/* Front_ResetDrawArea1P__Fv (0x8005948c) */
#include "names.h"
/* Byte-identical copy of Front_ResetDrawArea2P. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

extern u_long *pDoubleBuffer;
extern u_char *pPoly;

void Front_ResetDrawArea1P(void) {
	DR_AREA *p = (DR_AREA *)pPoly;
	RECT r;

	setRECT(&r, 0, 0, 640, 480);
	SetDrawArea(p, &r);

	{
		u_long *ot = pDoubleBuffer + 39;

		addPrim(ot, p);
	}

	pPoly = (u_char *)(p + 1);
}
